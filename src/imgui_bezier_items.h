#ifndef IMGUI_BEZIER_ITEMS_H
#define IMGUI_BEZIER_ITEMS_H
#pragma once

namespace ImFlow::detail {

/**
 * @brief Draw a sensible bezier between two points
 * @param p1 Starting point
 * @param p2 Ending point
 * @param color Color of the curve
 * @param thickness Thickness of the curve
 */
inline static void
smart_bezier(
        const ImVec2& p1,
        const ImVec2& p2,
        ImU32 color,
        float thickness);

/**
 * @brief Draw a sensible bezier whose color blends from one end to the other
 * @param p1 Starting point
 * @param p2 Ending point
 * @param color1 Color at the starting point
 * @param color2 Color at the ending point
 * @param thickness Thickness of the curve
 * @param blend Tightness of the color transition. 1 = linear, >1 concentrates it toward the middle, <1 spreads it
 */
inline static void smart_bezier_gradient(
    const ImVec2& p1,
    const ImVec2& p2,
    ImU32 color1,
    ImU32 color2,
    float thickness,
    float blend = 1.f);

/**
 * @brief Collider checker for smart_bezier
 * @details Projects the point "p" orthogonally onto the bezier curve and
 *	    checks if the distance is less than the given radius.
 * @param p Point to be tested
 * @param p1 Starting point of smart_bezier
 * @param p2 Ending point of smart_bezier
 * @param radius Lateral width of the hit box
 * @return [TRUE] if "p" is inside the collider
 *
 * Intended to be used in union with smart_bezier();
 */
inline static bool smart_bezier_collider(
    const ImVec2& p,
    const ImVec2& p1,
    const ImVec2& p2,
    float radius);

} // namespace ImFlow::detail

#include "imgui_bezier_items.inl"

#endif // IMGUI_BEZIER_ITEMS_H
