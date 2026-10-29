#include "NodeGroupRef.h"

#include "ImNodeFlow/NodeRef.h"
#include "ImNodeFlow/fwd.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace ImFlow::detail {

template <bool TIsConst>
NodeGroupRef<TIsConst>::NodeGroupRef(context_type& ctx, node_group_type& node_group)
    : m_ctx(ctx)
    , m_node_group(node_group)
{
}

template <bool TIsConst>
bool NodeGroupRef<TIsConst>::mouse_over_any_selected_node() const
{
    auto& node_group = m_node_group.get();
    return std::any_of(
        node_group.nodes.begin(),
        node_group.nodes.end(),
        [this](const auto& n) -> bool {
            ConstNodeRef node { m_ctx, *n };
            return node.isSelected() && node.isHovered();
        });
}

template <bool TIsConst>
uint64_t NodeGroupRef<TIsConst>::getNodesCount() const
{
    return m_node_group.get().nodes.size();
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::deselectAllNodes()
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& node : node_group.nodes) {
        NodeRef { m_ctx, *node }.setSelected(false);
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::provisionallySelectArea(ImRect clip_area)
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& elem : node_group.nodes) {
        NodeRef node { m_ctx, *elem };

        ImRect node_clip {
            node.getTLPos(),
            node.getTLPos() + node.getFullSize(),
        };

        if (clip_area.Overlaps(node_clip)) {
            node.setProvisionallySelected(true);
        }
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::selectArea(ImRect clip_area)
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& elem : node_group.nodes) {
        NodeRef node { m_ctx, *elem };

        ImRect node_clip {
            node.getTLPos(),
            node.getTLPos() + node.getFullSize(),
        };

        if (clip_area.Overlaps(node_clip)) {
            node.setSelected(true);
        }
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::calculate_inputs()
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& node : node_group.nodes) {
        NodeRef { m_ctx, *node }.calculate_inputs();
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::calculate_outputs()
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& node : node_group.nodes) {
        NodeRef { m_ctx, *node }.calculate_outputs();
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::prime_inputs()
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& node : node_group.nodes) {
        NodeRef { m_ctx, *node }.prime_inputs();
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::prime_outputs()
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    for (auto& node : node_group.nodes) {
        NodeRef { m_ctx, *node }.prime_outputs();
    }
}

template <bool TIsConst>
void NodeGroupRef<TIsConst>::update()
    requires(!TIsConst)
{
    auto& node_group = m_node_group.get();
    std::vector<std::unique_ptr<detail::node_t>> next_iteration_nodes;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (!node_group.nodes.empty()) {
        // We need this in order to render the nodes front to back
        ImDrawListSplitter splitter;
        splitter.Split(draw_list, node_group.nodes.size());
        for (size_t i = 0; i < node_group.nodes.size(); i++) {
            splitter.SetCurrentChannel(draw_list, node_group.nodes.size() - i - 1);
            NodeRef { m_ctx, *node_group.nodes[i] }.update();
        }
        splitter.Merge(draw_list);
    }

    size_t count_destroyed = 0;
    size_t count_activated = 0;
    size_t count_selected = 0;
    size_t count_idle = 0;

    for (auto& elem : node_group.nodes) {
        NodeRef node { m_ctx, *elem };

        if (node.toDestroy()) {
            ++count_destroyed;
            continue;
        }

        node.updatePublicStatus();

        if (node.isActivated()) {
            ++count_activated;
        } else if (node.isSelected()) {
            ++count_selected;
        } else {
            ++count_idle;
        }
    }

    next_iteration_nodes.resize(count_idle + count_selected + count_activated);

    size_t next_idx_activated = 0;
    size_t next_idx_selected = next_idx_activated + count_activated;
    size_t next_idx_idle = next_idx_selected + count_selected;

    for (auto& elem : node_group.nodes) {
        NodeRef node { m_ctx, *elem };
        if (node.toDestroy()) {
            node.destroy();
            continue;
        }

        if (node.isActivated()) {
            next_iteration_nodes[next_idx_activated] = std::move(elem);
            ++next_idx_activated;
        } else if (node.isSelected()) {
            next_iteration_nodes[next_idx_selected] = std::move(elem);
            ++next_idx_selected;
        } else {
            next_iteration_nodes[next_idx_idle] = std::move(elem);
            ++next_idx_idle;
        }
    }

    node_group.nodes = std::move(next_iteration_nodes);
}

template class NodeGroupRef<false>;
template class NodeGroupRef<true>;

} // namespace ImFlow::detail
