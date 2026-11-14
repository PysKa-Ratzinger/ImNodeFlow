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

    auto start_color = sourceNode().getStyle().header_bg;
    auto end_color = targetNode().getStyle().header_bg;

    ImVec2 start = sourceNode().getTLPos() + sourceNode().getFullSize() * ImVec2(1.F, 0.F);
    ImVec2 end = targetNode().getTLPos();

    start.y += ImGui::GetTextLineHeight();
    end.y += ImGui::GetTextLineHeight();

    auto p1 = start;
    auto p2 = start + ImVec2(20.F, 0.F);
    auto p3 = end - ImVec2(20.F, 0.F);
    auto p4 = end;

    link_base_update(
        ctx,
        link.base,
        link_base_update_args {
            .start_color = start_color,
            .end_color = end_color,
            .start_anchor = p1,
            .start = p2,
            .end = p3,
            .end_anchor = p4,
            .thickness = thickness,
            .drag_everything = sourceNode().wantsDrag()
                && targetNode().wantsDrag(),
            .draw_arrow = 0.6F,
        });

    if (link.base.marked_for_deletion) {
        this->destroy();
    }
}

template <bool TIsConst>
void NodeLinkRef<TIsConst>::prepareNextFrame()
    requires(!TIsConst)
{
    auto& link = m_link.get();
    link_base_prepare_next_frame(m_ctx, link.base);
}

template class NodeLinkRef<false>;
template class NodeLinkRef<true>;

} // namespace ImFlow::detail
