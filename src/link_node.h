#pragma once

#include "ImNodeFlow/detail/fwd.h"

#include "./link_base.h"

namespace ImFlow::detail {

struct link_node_t {
    link_base_t base;
    detail::node_t* source_node = nullptr;
    detail::node_t* target_node = nullptr;
};

} // namespace ImFlow::detail
