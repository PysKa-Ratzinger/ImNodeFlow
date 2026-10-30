#pragma once

#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "imgui_internal.h"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace ImFlow::detail {

template <bool TIsConst = false>
class NodeGroupArenaRef {
public:
    using context_type = std::conditional_t<TIsConst, const ImNodeFlow, ImNodeFlow>;
    using group_arena_type = std::conditional_t<TIsConst, const node_group_arena_t, node_group_arena_t>;

    NodeGroupArenaRef(context_type& ctx, group_arena_type& arena);

    [[nodiscard]] bool mouse_over_any_selected_node() const;

    [[nodiscard]] uint64_t getNodesCount() const;

    [[nodiscard]] uint64_t getSelectedNodesCount() const;

    void getNodes(std::vector<NodeRefImpl<TIsConst>>& out_vec) const;

    void getSelectedNodes(std::vector<NodeRefImpl<TIsConst>>& out_vec) const;

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

    void draw_outline(ConstNodeRef parentNode) const;

    void update()
        requires(!TIsConst);
    void prepareNextFrame()
        requires(!TIsConst);

private:
    std::reference_wrapper<context_type> m_ctx;
    std::reference_wrapper<group_arena_type> m_arena;
};

extern template class NodeGroupArenaRef<false>;
extern template class NodeGroupArenaRef<true>;

} // namespace ImFlow::detail
