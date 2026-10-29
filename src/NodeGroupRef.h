#pragma once

#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "imgui_internal.h"

#include <type_traits>

namespace ImFlow::detail {

template <bool TIsConst = false>
class NodeGroupRef {
public:
    using context_type = std::conditional_t<TIsConst, const ImNodeFlow, ImNodeFlow>;
    using node_group_type = std::conditional_t<TIsConst, const node_group_t, node_group_t>;

    NodeGroupRef(context_type& ctx, node_group_type& node_group);

    bool mouse_over_any_selected_node() const;

    uint64_t getNodesCount() const;

    void deselectAllNodes()
        requires(!TIsConst);
    void provisionallySelectArea(ImRect clip_area)
        requires(!TIsConst);
    void selectArea(ImRect clip_area)
        requires(!TIsConst);

    void calculate_inputs()
        requires(!TIsConst);
    void calculate_outputs()
        requires(!TIsConst);
    void prime_inputs()
        requires(!TIsConst);
    void prime_outputs()
        requires(!TIsConst);

    void update()
        requires(!TIsConst);

private:
    std::reference_wrapper<context_type> m_ctx;
    std::reference_wrapper<node_group_type> m_node_group;
};

extern template class NodeGroupRef<false>;
extern template class NodeGroupRef<true>;

} // namespace ImFlow::detail
