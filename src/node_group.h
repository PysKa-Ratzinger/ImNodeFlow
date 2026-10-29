#pragma once

#include "./node.h"

#include <memory>
#include <vector>

namespace ImFlow::detail {

struct node_group_t {
    std::vector<std::unique_ptr<node_t>> nodes;
};

} // namespace ImFlow::detail
