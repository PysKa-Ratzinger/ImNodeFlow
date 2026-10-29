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
    std::vector<std::unique_ptr<detail::node_t>> next_frame_nodes;
    next_frame_nodes.reserve(this->nodes.size());

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

    for (auto& node : this->nodes) {
        if (node->destroyed) {
            continue;
        }

        bool was_selected = node->selected;
        NodeRef { *node }.updatePublicStatus();
        bool is_selected = node->selected;

        if (!was_selected && is_selected) {
            next_frame_nodes.emplace_back(std::move(node));
        }
    }

    for (auto& node : this->nodes) {
        if (!node || node->destroyed) {
            continue;
        }

        if (node->selected) {
            next_frame_nodes.emplace_back(std::move(node));
        }
    }

    for (auto& node : this->nodes) {
        if (!node || node->destroyed) {
            continue;
        }

        next_frame_nodes.emplace_back(std::move(node));
    }
    this->nodes = std::move(next_frame_nodes);
}

} // namespace ImFlow::detail
