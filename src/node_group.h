#pragma once

#include "./node.h"

#include <memory>
#include <vector>

namespace ImFlow::detail {

/// A representation of the common fields every group will contain, including
/// the root group
struct node_group_arena_t {
    std::vector<std::unique_ptr<node_t>> nodes;
};

} // namespace ImFlow::detail
