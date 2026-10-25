#ifndef IMNODEFLOW_NODEINFO_H
#define IMNODEFLOW_NODEINFO_H
#pragma once

#include "ImNodeFlow/INodeBehaviour.h"
#include "ImNodeFlow/NodeStyle.h"

#include <memory>

namespace ImFlow {

/**
 * @brief User customizable options when creating a new Node
 */
struct NodeInfo {
    /// Node strategy for initialization and rendering
    std::unique_ptr<INodeBehaviour> behaviour;
    /// Style to apply to the node
    NodeStyle style = NodeStyles::default_style;
};

} // namespace ImFlow

#endif // IMNODEFLOW_NODEINFO_H
