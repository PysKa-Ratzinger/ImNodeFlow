#pragma once

#include "./node.h"

#include "imgui_internal.h"

#include <memory>
#include <vector>

namespace ImFlow::detail {

struct NodeGroup {
    std::vector<std::unique_ptr<node_t>> nodes;

    bool mouse_over_any_selected_node();

    uint64_t getNodesCount() const;

    void deselectAllNodes();
    void provisionallySelectArea(ImRect clip_area);
    void selectArea(ImRect clip_area);

    void calculate_inputs();
    void calculate_outputs();
    void prime_inputs();
    void prime_outputs();

    void update();
};

} // namespace ImFlow::detail
