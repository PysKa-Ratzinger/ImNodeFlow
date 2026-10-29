#pragma once

#include "ImNodeFlow/detail/fwd.h"

#include "imgui.h"

namespace ImFlow::detail {

void draw_bezier_from_pin_to_pos(
    ImDrawList* draw_list,
    pin_base_t const& pin_base,
    ImVec2 pos);

} // namespace ImFlow::detail
