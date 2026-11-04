#pragma once

#include "ImNodeFlow/detail/fwd.h"

#include "./link_base.h"

namespace ImFlow::detail {

struct link_pin_t {
    link_base_t base;
    detail::pin_base_t* out_pin = nullptr;
    detail::pin_base_t* in_pin = nullptr;
};

} // namespace ImFlow::detail
