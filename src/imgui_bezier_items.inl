#ifndef IMGUI_BEZIER_ITEMS_INL
#define IMGUI_BEZIER_ITEMS_INL
#pragma once

#include "imgui_bezier_math.h"

namespace ImFlow::detail {

inline void
smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
    float delta = distance * 0.45f;
    if (p2.x < p1.x) {
        delta += 0.2f * (p1.x - p2.x);
        delta = std::min(150.F, delta);
    }
    // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
    float vert = 0.f;
    ImVec2 p22 = p2 - ImVec2(delta, vert);
    ImVec2 p11 = p1 + ImVec2(delta, vert);
    dl->AddBezierCubic(p1, p11, p22, p2, color, thickness);
}

inline void
smart_bezier_gradient(
    const ImVec2& p1,
    const ImVec2& p2,
    ImU32 color1,
    ImU32 color2,
    float thickness,
    float blend)
{
    if (color1 == color2) {
        smart_bezier(p1, p2, color1, thickness);
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
    float delta = distance * 0.45f;
    if (p2.x < p1.x) {
        delta += 0.2f * (p1.x - p2.x);
        delta = std::min(150.F, delta);
    }
    float vert = 0.f;
    ImVec2 p22 = p2 - ImVec2(delta, vert);
    ImVec2 p11 = p1 + ImVec2(delta, vert);

    const int segments = 32;
    ImVec4 c1 = ImGui::ColorConvertU32ToFloat4(color1);
    ImVec4 c2 = ImGui::ColorConvertU32ToFloat4(color2);
    ImVec2 prev = p1;
    for (int i = 1; i <= segments; ++i) {
        float t = (float)i / (float)segments;
        float u = 1.f - t;
        ImVec2 pt(
            u * u * u * p1.x + 3 * u * u * t * p11.x + 3 * u * t * t * p22.x + t * t * t * p2.x,
            u * u * u * p1.y + 3 * u * u * t * p11.y + 3 * u * t * t * p22.y + t * t * t * p2.y);
        float tm = ((float)i - 0.5f) / (float)segments;
        if (blend != 1.f) // symmetric contrast curve around the midpoint
            tm = (tm < 0.5f) ? 0.5f * powf(2.f * tm, blend) : 1.f - 0.5f * powf(2.f * (1.f - tm), blend);
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImLerp(c1, c2, tm));
        dl->AddLine(prev, pt, col, thickness);
        prev = pt;
    }
}

inline bool
smart_bezier_collider(
    const ImVec2& p,
    const ImVec2& p1,
    const ImVec2& p2,
    float radius)
{
    float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
    float delta = distance * 0.45f;
    if (p2.x < p1.x) {
        delta += 0.2f * (p1.x - p2.x);
        delta = std::min(150.F, delta);
    }
    // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
    float vert = 0.f;
    ImVec2 p22 = p2 - ImVec2(delta, vert);
    ImVec2 p11 = p1 + ImVec2(delta, vert);
    return ImProjectOnCubicBezier(p, p1, p11, p22, p2).Distance < radius;
}

} // namespace ImFlow::detail

#include "imgui_bezier_items.inl"
#endif // IMGUI_BEZIER_ITEMS_INL
