#ifndef IMGUI_BEZIER_ITEMS_INL
#define IMGUI_BEZIER_ITEMS_INL
#pragma once

#include "imgui_bezier_items.h"

#include "./imgui_bezier_math.h"

#include "imgui.h"

#include <vector>

namespace ImFlow::detail {

std::vector<cubic_bezier_controls_t> smart_bezier_path_between_two_pins(
    const ImVec2& p1,
    const ImVec2& p2)
{
    std::vector<cubic_bezier_controls_t> result;

    if (p1.y == p2.y) {
        // Straight line
        result.emplace_back(cubic_bezier_controls_t { p1, p1, p2, p2 });
    } else {
        auto p11 = p1 + ImVec2(20.F, 0);
        auto p22 = p2 - ImVec2(20.F, 0);

        if (p22.x < p11.x) {
            float y_midpoint = (p1.y + p2.y) / 2.F;
            auto p1m = ImVec2(p11.x, y_midpoint);
            auto p2m = ImVec2(p22.x, y_midpoint);

            result.emplace_back(cubic_bezier_controls_t { p1, p1, p11, p11 });
            result.emplace_back(cubic_bezier_controls_t { p11, p11, p1m, p1m });
            result.emplace_back(cubic_bezier_controls_t { p1m, p1m, p2m, p2m });
            result.emplace_back(cubic_bezier_controls_t { p2m, p2m, p22, p22 });
            result.emplace_back(cubic_bezier_controls_t { p22, p22, p2, p2 });
        } else {
            p22 = ImVec2(p11.x, p22.y);

            result.emplace_back(cubic_bezier_controls_t { p1, p1, p11, p11 });
            result.emplace_back(cubic_bezier_controls_t { p11, p11, p22, p22 });
            result.emplace_back(cubic_bezier_controls_t { p22, p22, p2, p2 });
        }
    }

    return result;
}

void smart_bezier(
    ImDrawList* draw_list,
    cubic_bezier_controls_t const& line,
    ImU32 color,
    float thickness)
{
    draw_list->AddBezierCubic(
        line.p1,
        line.p2,
        line.p3,
        line.p4,
        color,
        thickness);
}

void smart_bezier_gradient(
    ImDrawList* draw_list,
    cubic_bezier_controls_t const& line,
    ImU32 color1,
    ImU32 color2,
    float thickness,
    float blend)
{
    if (color1 == color2) {
        smart_bezier(draw_list, line, color1, thickness);
        return;
    }

    ImVec2 p1 = line.p1;
    ImVec2 p11 = line.p2;
    ImVec2 p22 = line.p3;
    ImVec2 p2 = line.p4;

    int vtx_buffer_old_size = draw_list->VtxBuffer.Size;
    draw_list->AddBezierCubic(
        line.p1,
        line.p2,
        line.p3,
        line.p4,
        color1,
        thickness);
    int vtx_buffer_new_size = draw_list->VtxBuffer.Size;

    ImVec4 c1 = ImGui::ColorConvertU32ToFloat4(color1);
    ImVec4 c2 = ImGui::ColorConvertU32ToFloat4(color2);

    for (int i = vtx_buffer_old_size; i != vtx_buffer_new_size; i++) {
        float alpha = float(i - vtx_buffer_old_size) / float(vtx_buffer_new_size - vtx_buffer_old_size - 1);
        if (blend != 1.f) // symmetric contrast curve around the midpoint
            alpha = (alpha < 0.5f) ? 0.5f * powf(2.f * alpha, blend) : 1.f - 0.5f * powf(2.f * (1.f - alpha), blend);
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImLerp(c1, c2, alpha));
        draw_list->VtxBuffer.Data[i].col = col;
    }
}

bool smart_bezier_collider(
    const ImVec2& p,
    cubic_bezier_controls_t const& ctrl,
    float radius)
{
    return ImProjectOnCubicBezier(p, ctrl.p1, ctrl.p2, ctrl.p3, ctrl.p4).Distance < radius;
}

} // namespace ImFlow::detail

#endif // IMGUI_BEZIER_ITEMS_INL
