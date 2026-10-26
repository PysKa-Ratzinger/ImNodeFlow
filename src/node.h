#ifndef IMNODEFLOW_NODE_H
#define IMNODEFLOW_NODE_H
#pragma once

#include "ImNodeFlow/NodeStyle.h"
#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "./DynamicPin.h"

#include "imgui.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImFlow::detail {

struct node_t {
    std::reference_wrapper<ImNodeFlow> ctx;

    NodeUID uid;
    std::string title;
    ImVec2 pos;
    ImVec2 posTarget;
    ImVec2 size;
    ImVec2 fullSize;
    NodeStyle style;
    bool selected = false;
    bool selectedNext = false;
    bool provisionallySelected = false;
    bool provisionallySelectedNext = false;
    bool dragged = false;
    bool destroyed = false;
    bool indestructable = false;

    std::unique_ptr<INodeBehaviour> behaviour;

    std::vector<std::unique_ptr<detail::pin_base_t>> ins;
    std::vector<DynamicPin> dynamicIns;
    std::vector<std::unique_ptr<detail::pin_base_t>> outs;
    std::vector<DynamicPin> dynamicOuts;
};

} // namespace ImFlow::detail

#endif // IMNODEFLOW_NODE_H
