#ifndef IMNODEFLOW_PINSTYLE_H
#define IMNODEFLOW_PINSTYLE_H
#pragma once

#include "imgui.h"

namespace ImFlow {

/**
 * @brief Extra pin's style setting
 */
struct PinStyleExtras {
    /// @brief Top and bottom spacing
    ImVec2 padding = ImVec2(3.f, 1.f);
    /// @brief Border and background corner rounding
    float bg_radius = 8.f;
    /// @brief Border thickness
    float border_thickness = 1.f;
    /// @brief Background color
    ImU32 bg_color = IM_COL32(23, 16, 16, 0);
    /// @brief Background color when hovered
    ImU32 bg_hover_color = IM_COL32(100, 100, 255, 70);
    /// @brief Border color
    ImU32 border_color = IM_COL32(255, 255, 255, 0);

    /// @brief Link thickness
    float link_thickness = 2.6f;
    /// @brief Link thickness when dragged
    float link_dragged_thickness = 2.2f;
    /// @brief Link thickness when hovered
    float link_hovered_thickness = 3.5f;
    /// @brief Thickness of the outline of a selected link
    float link_selected_outline_thickness = 1.f;
    /// @brief Color of the outline of a selected link
    ImU32 outline_color = IM_COL32(80, 20, 255, 200);

    /// @brief Spacing between pin content and socket
    float socket_padding = 6.6f;
};

/**
 * @brief Defines the visual appearance of a pin
 */
class PinStyle {
public:
    /// @brief Socket and link color
    ImU32 color;
    /// @brief Socket shape ID
    int socket_shape;
    /// @brief Socket radius
    float socket_radius;
    /// @brief Socket radius when hovered
    float socket_hovered_radius;
    /// @brief Socket radius when connected
    float socket_connected_radius;
    /// @brief Socket outline thickness when empty
    float socket_thickness;
    /// @brief List of less common properties
    PinStyleExtras extra;
};

namespace PinStyles {

    /// @brief Default cyan style
    static constexpr PinStyle cyan { IM_COL32(87, 155, 185, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default green style
    static constexpr PinStyle green { IM_COL32(90, 191, 93, 255), 4, 4.f, 4.67f, 4.2f, 1.3f };
    /// @brief Default blue style
    static constexpr PinStyle blue { IM_COL32(90, 117, 191, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default brown style
    static constexpr PinStyle brown { IM_COL32(191, 134, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default red style
    static constexpr PinStyle red { IM_COL32(191, 90, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default white style
    static constexpr PinStyle white { IM_COL32(255, 255, 255, 255), 5, 4.f, 4.67f, 4.2f, 1.f };

    static inline constexpr PinStyle default_style = cyan;

}

} // namespace ImFlow

#endif // IMNODEFLOW_PINSTYLE_H
