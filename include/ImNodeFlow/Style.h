#ifndef IMNODEFLOW_STYLE_H
#define IMNODEFLOW_STYLE_H
#pragma once

#include "imgui.h"

namespace ImFlow {

/**
 * @brief Grid's the color parameters
 */
struct InfColors {
    /// @brief Background of the grid
    ImU32 background = IM_COL32(33, 41, 45, 255);
    /// @brief Main lines of the grid
    ImU32 grid = IM_COL32(200, 200, 200, 40);
    /// @brief Secondary lines
    ImU32 subGrid = IM_COL32(200, 200, 200, 10);
};

/**
 * @brief ALl the grid's appearance parameters. Sizes + Colors
 */
struct InfStyler {
    /// @brief Size of main grid
    float grid_size = 50.f;
    /// @brief Sub-grid divisions for Node snapping
    float grid_subdivisions = 5.f;
    /// @brief Tightness of gradient links. 1 = linear, >1 concentrates the color transition toward the middle, <1 spreads it out
    float link_gradient_blend = 1.f;
    /// @brief ImNodeFlow colors
    InfColors colors;
};

} // namespace ImFlow

#endif // IMNODEFLOW_STYLE_H
