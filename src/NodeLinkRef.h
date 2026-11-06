#pragma once

#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "./imgui_bezier_items.h"
#include "./link_base.h"

#include "imgui.h"

#include <array>
#include <functional>
#include <type_traits>

namespace ImFlow::detail {

template <bool TIsConst = false>
class NodeLinkRef {
public:
    using context_type = std::conditional_t<TIsConst, const ImNodeFlow, ImNodeFlow>;
    using link_type = std::conditional_t<TIsConst, const link_node_t, link_node_t>;

    NodeLinkRef(context_type& ctx, link_type& link);

    static link_grid_path_t
    getPathToMouse(
        ImNodeFlow& ctx,
        ConstNodeRef sourceNode);

    [[nodiscard]] bool isAlive() const;

    [[nodiscard]] NodeRefImpl<TIsConst> sourceNode() const;
    [[nodiscard]] NodeRefImpl<TIsConst> targetNode() const;

    void connect(NodeRef source_node, NodeRef target_node)
        requires(!TIsConst);

    void destroy()
        requires(!TIsConst);

    void update()
        requires(!TIsConst);

    void prepareNextFrame()
        requires(!TIsConst);

private:
    std::reference_wrapper<context_type> m_ctx;
    std::reference_wrapper<link_type> m_link;
};

extern template class NodeLinkRef<false>;
extern template class NodeLinkRef<true>;

} // namespace ImFlow::detail
