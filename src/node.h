#ifndef IMNODEFLOW_NODE_H
#define IMNODEFLOW_NODE_H
#pragma once

#include "ImNodeFlow/NodeStyle.h"
#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "./dynamic_pin.h"
#include "./selectable.h"

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>

namespace ImFlow::detail {

struct node_t {
    NodeUID uid;
    /// When NULL, the node belongs in the root group
    node_t* parent_node = nullptr;

    std::string title;
    ImVec2 pos;
    ImVec2 posTarget;
    ImVec2 posBackup;
    ImVec2 size;
    ImVec2 fullSize;
    NodeStyle style;
    detail::Selectable selectable;
    bool destroyed = false;
    bool dragging = false;
    bool indestructable = false;
    bool allowInboundLinksToOtherNodes = true;
    bool allowOutboundLinksToOtherNodes = true;

    std::unique_ptr<node_group_arena_t> child_arena;
    std::unique_ptr<INodeBehaviour> behaviour;

    std::vector<detail::link_node_t*> links;

    std::vector<std::unique_ptr<detail::pin_base_t>> ins;
    std::vector<std::unique_ptr<detail::dynamic_pin_t>> dynamicIns;
    std::vector<std::unique_ptr<detail::pin_base_t>> outs;
    std::vector<std::unique_ptr<detail::dynamic_pin_t>> dynamicOuts;
};

} // namespace ImFlow::detail

#endif // IMNODEFLOW_NODE_H
