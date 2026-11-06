#include "imgui_custom_widgets.h"

#include "imgui.h"
#include "imgui_internal.h"

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
    bool* out_released)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 delta = mouse_pos - screen_pos;
    double dist_squared = delta.x * delta.x + delta.y * delta.y;
    double radius_squared = radius * radius;

    bool is_hovered = dist_squared <= radius_squared;
    bool is_held = false;
    bool is_clicked = false;
    bool is_released = false;

    if (is_hovered || held) {
        ImRect always_valid_bb {
            mouse_pos - ImVec2(1.F, 1.F),
            mouse_pos + ImVec2(1.F, 1.F),
        };

        ImGui::PushID(id);
        if (ImGui::ItemAdd(always_valid_bb, id)) {
            is_clicked = ImGui::ButtonBehavior(
                always_valid_bb,
                id,
                &is_hovered,
                &is_held,
                ImGuiButtonFlags_PressedOnClick);
            is_released = ImGui::IsItemDeactivated();
        }
        ImGui::PopID();
    }

    ImU32 bg_color = IM_COL32(150, 150, 150, 255);
    if (is_hovered || selected) {
        bg_color = IM_COL32(240, 240, 240, 255);
    }

    draw_list->AddCircleFilled(screen_pos, radius, bg_color);
    draw_list->AddCircle(screen_pos, radius, IM_COL32(0, 0, 0, 255));

    if (out_hovered != nullptr) {
        *out_hovered = is_hovered;
    }
    if (out_held != nullptr) {
        *out_held = is_held;
    }
    if (out_released != nullptr) {
        *out_released = is_released;
    }

    return is_clicked;
}

} // namespace ImFlow::detail
