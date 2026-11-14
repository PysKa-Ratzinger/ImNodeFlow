#include "imgui_bezier_widgets.h"

#include "./imgui_bezier_items.h"
#include "./imgui_colors.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace ImFlow::detail {

bool smart_bezier_widget(
    ImGuiID item_id,
    ImDrawList* draw_list,
    cubic_bezier_controls_t const& line,
    ImU32 color1,
    ImU32 color2,
    bool selected,
    bool is_hovered,
    bool* out_hovered,
    bool* out_double_clicked,
    bool* out_released,
    float thickness,
    float blend)
{
    auto mouse_pos = ImGui::GetMousePos();
    bool is_segment_hovered = detail::smart_bezier_collider(mouse_pos, line, 2.5);
    bool is_clicked = false;
    bool is_released = false;
    bool is_double_clicked = false;

    if (is_segment_hovered) {
        ImRect always_valid_bb {
            mouse_pos - ImVec2(1.F, 1.F),
            mouse_pos + ImVec2(1.F, 1.F),
        };

        ImGui::PushID(item_id);
        if (ImGui::ItemAdd(always_valid_bb, item_id)) {
            is_double_clicked = ImGui::ButtonBehavior(
                always_valid_bb,
                item_id,
                &is_hovered,
                nullptr,
                ImGuiButtonFlags_PressedOnDoubleClick);
            is_released = ImGui::IsItemDeactivated();
            is_clicked = ImGui::IsItemClicked();
        }
        ImGui::PopID();
    }

    auto start_color = color1;
    auto end_color = color2;

    if (selected) {
        double t = ImGui::GetTime();

        start_color = detail::get_selected_color(t * 2.F);
        end_color = detail::get_selected_color((t * 2.F) + 2.F);
    }

    smart_bezier_gradient(
        draw_list,
        line,
        start_color,
        end_color,
        thickness,
        blend);

    if (out_hovered != nullptr) {
        *out_hovered = is_segment_hovered;
    }
    if (out_double_clicked != nullptr) {
        *out_double_clicked = is_double_clicked;
    }
    if (out_released != nullptr) {
        *out_released = is_released;
    }

    return is_clicked;
}

} // namespace ImFlow::detail
