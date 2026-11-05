#include "NodeLinkRef.h"

#include "ImNodeFlow/Context.h"
#include "ImNodeFlow/NodeRef.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "./imgui_bezier_items.h"
#include "./imgui_bezier_widgets.h"
#include "./link_base.h"
#include "./link_node.h"
#include "./node.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <vector>

namespace ImFlow::detail {

template <bool TIsConst>
NodeLinkRef<TIsConst>::NodeLinkRef(context_type& ctx, link_type& link)
    : m_ctx(ctx)
    , m_link(link)
{
}

template <bool TIsConst>
bool NodeLinkRef<TIsConst>::isAlive() const
{
    auto& link = m_link.get();
    return !link.base.marked_for_deletion
        && link.source_node != nullptr
        && link.target_node != nullptr;
}

template <bool TIsConst>
NodeRefImpl<TIsConst> NodeLinkRef<TIsConst>::sourceNode() const
{
    auto& link = m_link.get();
    return NodeRefImpl<TIsConst> { m_ctx, *link.source_node };
}

template <bool TIsConst>
NodeRefImpl<TIsConst> NodeLinkRef<TIsConst>::targetNode() const
{
    auto& link = m_link.get();
    return NodeRefImpl<TIsConst> { m_ctx, *link.target_node };
}

template <bool TIsConst>
void NodeLinkRef<TIsConst>::connect(NodeRef source_node, NodeRef target_node)
    requires(!TIsConst)
{
    this->destroy();

    link_node_t& link = m_link.get();
    link.base.marked_for_deletion = false;
    link.source_node = &static_cast<node_t&>(source_node);
    link.target_node = &static_cast<node_t&>(target_node);

    assert(link.source_node != link.target_node
        && "Links cannot be created starting and ending in the same node");

    link.source_node->links.emplace_back(&link);
    link.target_node->links.emplace_back(&link);
}

template <bool TIsConst>
void NodeLinkRef<TIsConst>::destroy()
    requires(!TIsConst)
{
    link_node_t& link = m_link.get();
    if (link.source_node != nullptr) {
        auto iter = std::find(
            std::begin(link.source_node->links),
            std::end(link.source_node->links),
            &link);
        if (iter != std::end(link.source_node->links)) {
            link.source_node->links.erase(iter);
        }
        link.source_node = nullptr;
    }
    if (link.target_node != nullptr) {
        auto iter = std::find(
            std::begin(link.target_node->links),
            std::end(link.target_node->links),
            &link);
        if (iter != std::end(link.target_node->links)) {
            link.target_node->links.erase(iter);
        }
        link.target_node = nullptr;
    }
    link.base.marked_for_deletion = true;
}

template <bool TIsConst>
void NodeLinkRef<TIsConst>::update()
    requires(!TIsConst)
{
    ImNodeFlow& ctx = m_ctx.get();
    detail::link_node_t& link = m_link.get();

    if (link.base.marked_for_deletion) {
        return;
    }

    float thickness = link.base.hovered
        ? sourceNode().getStyle().link_hovered_thickness
        : sourceNode().getStyle().link_thickness;

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        link.base.selected = false;
    }

    auto start_color = sourceNode().getStyle().header_bg;
    auto end_color = targetNode().getStyle().header_bg;

    auto path = detail::generate_path_from_node_to_node(
        ctx,
        sourceNode(),
        targetNode(),
        link.base.waypoints);
    bool pressed = draw_path(
        m_ctx,
        ImGui::GetWindowDrawList(),
        ImGui::GetID(&link),
        path,
        start_color,
        end_color,
        thickness,
        link.base.selected,
        link.base.hovered,
        &link.base.hovered);

    link.base.selected = link.base.selected || pressed;

    if (link.base.selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        this->destroy();
    }
}

} // namespace ImFlow::detail
