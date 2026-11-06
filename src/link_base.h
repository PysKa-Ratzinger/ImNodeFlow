#pragma once

#include "ImNodeFlow/fwd.h"

#include "imgui.h"

#include <vector>

namespace ImFlow::detail {

struct link_waypoint_t {
    /// Position in a uv projection, where uv are the x and y components
    /// of the vector (end - start).
    /// The end and start vectors are only known when the link_base_update()
    /// function is called.
    ImVec2 uv_pos;
    bool held = false;
    bool selected = false;
};

struct link_base_t {
    std::vector<link_waypoint_t> waypoints;
    bool marked_for_deletion = false;
    bool hovered = false;
    bool selected = false;
};

struct link_grid_path_t {
    struct coord_t {
        ImVec2 pos;
        int waypoint_index;
    };

    /// All points are in grid coordinates, not screen coordinates
    std::vector<coord_t> points;
};

struct link_base_update_args {
    ImU32 start_color;
    ImU32 end_color;
    ImVec2 start_anchor;
    ImVec2 start;
    ImVec2 end;
    ImVec2 end_anchor;
    float thickness;
};

void link_base_update(
    ImNodeFlow& ctx,
    link_base_t& link,
    link_base_update_args args);

link_grid_path_t generate_path_from_node_to_pos(
    ImNodeFlow& ctx,
    ConstNodeRef node,
    ImVec2 pos);

link_grid_path_t generate_path_from_pin_to_pos(
    ImNodeFlow& ctx,
    ConstPinRef pin,
    ImVec2 pos);

link_grid_path_t generate_path(
    ImNodeFlow& ctx,
    ImVec2 start_point_anchor,
    ImVec2 start_point,
    ImVec2 end_point,
    ImVec2 end_point_anchor,
    std::vector<link_waypoint_t> const& waypoints = { });

bool draw_path(
    ImNodeFlow& ctx,
    ImDrawList* draw_list,
    ImGuiID path_id,
    link_grid_path_t const& path,
    ImU32 start_color,
    ImU32 end_color,
    float thickness = 1.F,
    bool selected = false,
    bool hovered = false,
    bool* out_hovered = nullptr,
    int* out_double_clicked = nullptr);

} // namespace ImFlow::detail
