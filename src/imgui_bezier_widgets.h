#pragma once

#include "./imgui_bezier_items.h"

#include "imgui.h"

namespace ImFlow::detail {

/**
 * @brief Bezier curve widget
 * @details Draws a bezier curve and processes GUI interactions with ImGui
 * @param id ImguiID for the current curve
 * @param draw_list Pointer to the drawlist where to draw
 * @param line Bezier curve description
 * @param color1 Color at the starting point
 * @param color2 Color at the ending point
 * @param selected Whether or not the curve is currently selected
 * @param is_hovered Whether or not the link should be drawn with the hovered
 * style
 * @param out_hovered If not null, will save whether the mouse is currently
 * hovering over the bezier curve or not.
 * @param out_double_clicked If not null, will receive whether or not the
 * bezier curve was double clicked
 * @param out_released If not null, will receive whether the mouse was released
 * @param thickness Thickness of the curve
 * @param blend Tightness of the color transition. 1 = linear, >1 concentrates it toward the middle, <1 spreads it
 * @return [TRUE] if the bezier curve was pressed by the user
 */
bool smart_bezier_widget(
    ImGuiID id,
    ImDrawList* draw_list,
    cubic_bezier_controls_t const& line,
    ImU32 color1,
    ImU32 color2,
    bool selected,
    bool is_hovered,
    bool* out_hovered,
    bool* out_double_clicked = nullptr,
    bool* out_released = nullptr,
    float thickness = 2.F,
    float blend = 1.f);

} // namespace ImFlow::detail
