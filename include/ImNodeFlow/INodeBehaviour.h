#ifndef IMNODEFLOW_INODEBEHAVIOUR_H
#define IMNODEFLOW_INODEBEHAVIOUR_H
#pragma once

#include "./fwd.h"

#include <memory>

namespace ImFlow {

/**
 * @brief Node behaviour strategy.
 *
 * This is what allows the user to customize the node's appearance and
 * behaviour
 */
class INodeBehaviour {
public:
    virtual ~INodeBehaviour();

    /**
     * @brief Called immediately after a node is constructed.
     */
    virtual void setup(NodeRef node);

    /**
     * @brief Called each frame. Responsible for the appearance of the
     * node's inner content
     */
    virtual void draw(NodeRef node);

    /**
     * @brief Called during a node cloning event. If not implemented or
     * nullptr is returned, the cloning event fails for the corresponding
     * node.
     */
    [[nodiscard]] virtual std::unique_ptr<INodeBehaviour> clone()
    {
        return { };
    }
};

} // namespace ImFlow

#endif // IMNODEFLOW_INODEBEHAVIOUR_H
