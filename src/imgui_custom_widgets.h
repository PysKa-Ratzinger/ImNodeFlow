#pragma once

namespace ImFlow::detail {

bool drag_pin_widget(
    ImGuiID id,
    ImDrawList* draw_list,
    ImVec2 screen_pos,
    float radius,
    bool held,
    bool selected,
    bool* out_hovered,
    bool* out_held,
    bool* out_released);

} // namespace ImFlow::detail
