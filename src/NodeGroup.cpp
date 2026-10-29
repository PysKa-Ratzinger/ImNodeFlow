#include "NodeGroup.h"

#include "ImNodeFlow/NodeRef.h"
#include "ImNodeFlow/fwd.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace ImFlow::detail {

bool NodeGroup::mouse_over_any_selected_node()
{
    return std::any_of(
        this->nodes.begin(),
        this->nodes.end(),
        [](const auto& n) -> bool {
            ConstNodeRef node { *n };
            return node.isSelected() && node.isHovered();
        });
}

uint64_t NodeGroup::getNodesCount() const
{
    return this->nodes.size();
}

void NodeGroup::deselectAllNodes()
{
    for (auto& node : this->nodes) {
        NodeRef { *node }.setSelected(false);
    }
}

void NodeGroup::provisionallySelectArea(ImRect clip_area)
{
    for (auto& elem : this->nodes) {
        NodeRef node { *elem };

        ImRect node_clip {
            node.getTLPos(),
            node.getTLPos() + node.getFullSize(),
        };

        if (clip_area.Overlaps(node_clip)) {
            node.setProvisionallySelected(true);
        }
    }
}

void NodeGroup::selectArea(ImRect clip_area)
{
    for (auto& elem : this->nodes) {
        NodeRef node { *elem };

        ImRect node_clip {
            node.getTLPos(),
            node.getTLPos() + node.getFullSize(),
        };

        if (clip_area.Overlaps(node_clip)) {
            node.setSelected(true);
        }
    }
}

void NodeGroup::calculate_inputs()
{
    for (auto& node : this->nodes) {
        NodeRef { *node }.calculate_inputs();
    }
}

void NodeGroup::calculate_outputs()
{
    for (auto& node : this->nodes) {
        NodeRef { *node }.calculate_outputs();
    }
}

void NodeGroup::prime_inputs()
{
    for (auto& node : this->nodes) {
        NodeRef { *node }.prime_inputs();
    }
}

void NodeGroup::prime_outputs()
{
    for (auto& node : this->nodes) {
        NodeRef { *node }.prime_outputs();
    }
}

void NodeGroup::update()
{
    std::vector<std::unique_ptr<detail::node_t>> next_iteration_nodes;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (!this->nodes.empty()) {
        // We need this in order to render the nodes front to back
        ImDrawListSplitter splitter;
        splitter.Split(draw_list, this->nodes.size());
        for (size_t i = 0; i < this->nodes.size(); i++) {
            splitter.SetCurrentChannel(draw_list, this->nodes.size() - i - 1);
            NodeRef { *this->nodes[i] }.update();
        }
        splitter.Merge(draw_list);
    }

    size_t count_destroyed = 0;
    size_t count_activated = 0;
    size_t count_selected = 0;
    size_t count_idle = 0;

    for (auto& node : this->nodes) {
        if (node->destroyed) {
            ++count_destroyed;
            continue;
        }

        NodeRef { *node }.updatePublicStatus();
        bool is_activated = !node->wasSelected && node->selected;

        if (is_activated) {
            ++count_activated;
        } else if (node->selected) {
            ++count_selected;
        } else {
            ++count_idle;
        }
    }

    next_iteration_nodes.resize(count_idle + count_selected + count_activated);

    size_t next_idx_activated = 0;
    size_t next_idx_selected = next_idx_activated + count_activated;
    size_t next_idx_idle = next_idx_selected + count_selected;

    for (auto& node : this->nodes) {
        if (node->destroyed) {
            continue;
        }

        bool is_activated = !node->wasSelected && node->selected;

        if (is_activated) {
            next_iteration_nodes[next_idx_activated] = std::move(node);
            ++next_idx_activated;
        } else if (node->selected) {
            next_iteration_nodes[next_idx_selected] = std::move(node);
            ++next_idx_selected;
        } else {
            next_iteration_nodes[next_idx_idle] = std::move(node);
            ++next_idx_idle;
        }
    }

    this->nodes = std::move(next_iteration_nodes);
}

} // namespace ImFlow::detail
