#pragma once

#include "ImNodeFlow/fwd.h"

#include "imgui.h"

namespace ImFlow::detail {

void draw_path_from_pin_to_pos(
    ImDrawList* draw_list,
    ImNodeFlow& ctx,
    ConstPinRef pin,
    ImVec2 grid_pos);

} // namespace ImFlow::detail
