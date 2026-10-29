#pragma once

#include "ImNodeFlow/detail/fwd.h"

#include "./pin_base.h"

#include <cstdint>
#include <memory>

namespace ImFlow::detail {

struct dynamic_pin_t {
    detail::pin_base_t pin;
    uint32_t ttl = 1;
};

} // namespace ImFlow::detail
