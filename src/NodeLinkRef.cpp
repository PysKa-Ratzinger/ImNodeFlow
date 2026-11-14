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

namespace {

    struct node_link_socket_t {
        ImVec2 point;
        ImVec2 pivot;
    };

    node_link_socket_t get_socket(ImRect source_bb, ImRect target_bb, bool inverted)
    {
        auto t_bl = target_bb.GetBL();
        auto t_tr = target_bb.GetTR();
        auto s_bl = source_bb.GetBL();
        auto s_tr = source_bb.GetTR();

        ImVec2 res { s_bl };
        ImVec2 pivot { 0.F, 0.F };

        bool t_r_s = t_bl.x > s_tr.x; // target right of source
        bool t_l_s = t_tr.x < s_bl.x; // target left of source
        bool t_a_s = t_bl.y > s_tr.y; // target above source
        bool t_b_s = t_tr.y < s_bl.y; // target below source

        if (t_r_s) {
            res.x = s_tr.x;
            pivot.x = 20.F;
        } else if (t_l_s) {
            res.x = s_bl.x;
            pivot.x = -20.F;
        } else {
            pivot.x = 0.F;
            if (t_a_s ^ inverted) {
                res.x = std::max(s_bl.x, t_bl.x);
            } else {
                res.x = std::min(s_tr.x, t_tr.x);
            }
        }

        if (t_bl.y > s_tr.y) {
            res.y = s_tr.y;
            pivot.y = 20.F;
        } else if (t_tr.y < s_bl.y) {
            res.y = s_bl.y;
            pivot.y = -20.F;
        } else {
            pivot.y = 0.F;
            if (t_l_s ^ inverted) {
                res.y = std::max(s_bl.y, t_bl.y);
            } else {
                res.y = std::min(s_tr.y, t_tr.y);
            }
        }

        return {
            .point = res,
            .pivot = res + pivot,
        };
    }

} // anonymous namespace

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

    auto arrow_size = sourceNode().getStyle().arrow_size;

    auto start_color = sourceNode().getStyle().header_bg;
    auto end_color = targetNode().getStyle().header_bg;

    ImVec2 start = sourceNode().getTLPos() + sourceNode().getFullSize() * ImVec2(1.F, 0.F);
    ImVec2 end = targetNode().getTLPos();

    start.y += ImGui::GetTextLineHeight();
    end.y += ImGui::GetTextLineHeight();

    auto source_bb_p = sourceNode().getBB();
    auto target_bb_p = targetNode().getBB();

    ImRect source_bb { source_bb_p.first, source_bb_p.second };
    ImRect target_bb { target_bb_p.first, target_bb_p.second };

    ImRect source_t_bb = target_bb;
    ImRect target_t_bb = source_bb;

    if (!link.base.waypoints.empty()) {
        auto p0 = link.base.waypoints[0].grid_pos;
        auto p1 = link.base.waypoints[link.base.waypoints.size() - 1].grid_pos;

        source_t_bb = ImRect { p0, p0 };
        target_t_bb = ImRect { p1, p1 };
    }

    auto socket_start = get_socket(source_bb, source_t_bb, false);
    auto socket_end = get_socket(target_bb, target_t_bb, true);

    auto p1 = socket_start.point;
    auto p2 = socket_start.pivot;
    auto p3 = socket_end.pivot;
    auto p4 = socket_end.point;

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
            .arrow_size = arrow_size,
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
