#include "ImNodeFlow/INodeBehaviour.h"
#include "ImNodeFlow/NodeRef.h"
#include "ImNodeFlow/callback_types.h"
#include "ImNodeFlow/fwd.h"

#include <utility>

namespace ImFlow::detail {

/**
 * @brief Helper struct for creating a node from a lambda
 */
struct NodeWrapper : public INodeBehaviour {
    lambda_draw_func_t m_draw_func;

    explicit NodeWrapper(lambda_draw_func_t draw_func)
        : m_draw_func(std::move(draw_func))
    {
    }

    void draw(NodeRef node) override { m_draw_func(node); }
};

} // namespace ImFlow::detail
