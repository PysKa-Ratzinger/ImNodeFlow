#pragma once

#include "ImNodeFlow/detail/fwd.h"

namespace ImFlow::detail {

struct pin_link_t {
    detail::pin_base_t* out_pin = nullptr;
    detail::pin_base_t* in_pin = nullptr;
    bool marked_for_deletion = false;
    bool hovered = false;
    bool selected = false;
};

} // namespace ImFlow::detail
