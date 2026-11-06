#include "link_base.h"

#include "ImNodeFlow/Context.h"

#include "./imgui_bezier_items.h"
#include "./imgui_bezier_widgets.h"

#include "ImNodeFlow/fwd.h"
#include "imgui.h"

#include <vector>

namespace ImFlow::detail {

namespace {

    void process_next_point(
        link_grid_path_t& result,
        link_grid_path_t::coord_t next_pt)
    {
        auto last_pt = result.points.back();

        // If no straight line is possible
        if (next_pt.pos.x != last_pt.pos.x && next_pt.pos.y != last_pt.pos.y) {
            // let's do a Z-bend.
            if (next_pt.pos.x > last_pt.pos.x) {
                // In case this next point is to the right of the previous one,
                // the Z-bend will first move through the X axis until midway,
                // move Y axis all the way, then finish moving through the X
                // axis.
                float x_midpoint = (last_pt.pos.x + next_pt.pos.x) / 2.F;
                result.points.emplace_back(link_grid_path_t::coord_t {
                    .pos = { x_midpoint, last_pt.pos.y },
                    .waypoint_index = next_pt.waypoint_index,
                });
                result.points.emplace_back(link_grid_path_t::coord_t {
                    .pos = { x_midpoint, next_pt.pos.y },
                    .waypoint_index = next_pt.waypoint_index,
                });
            } else {
                // When going to the left side, let's do the opposite. Let's start
                // by going through the Y axis halfway, the the whole X axis, then
                // the rest of the Y axis.
                float y_midpoint = (last_pt.pos.y + next_pt.pos.y) / 2.F;
                result.points.emplace_back(link_grid_path_t::coord_t {
                    .pos = { last_pt.pos.x, y_midpoint },
                    .waypoint_index = next_pt.waypoint_index,
                });
                result.points.emplace_back(link_grid_path_t::coord_t {
                    .pos = { next_pt.pos.x, y_midpoint },
                    .waypoint_index = next_pt.waypoint_index,
                });
            }
        }

        result.points.emplace_back(next_pt);
    }

} // anonymous namespace

void link_base_update(
    ImNodeFlow& ctx,
    link_base_t& link,
    link_base_update_args args)
{
    auto path = detail::generate_path(
        ctx,
        args.start_anchor,
        args.start,
        args.end,
        args.end_anchor,
        link.waypoints);

    int double_clicked = -1;
    bool clicked = draw_path(
        ctx,
        ImGui::GetWindowDrawList(),
        ImGui::GetID(&link),
        path,
        args.start_color,
        args.end_color,
        args.thickness,
        link.selected,
        link.hovered,
        &link.hovered,
        &double_clicked);

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        link.selected = false;
    }

    link.selected = link.selected || clicked;

    if (double_clicked >= 0 && double_clicked <= link.waypoints.size()) {
        ImVec2 uv_axis = args.end - args.start;
        ImVec2 waypoint_pos = ctx.screen2grid(ImGui::GetMousePos());
        waypoint_pos.x = uv_axis.x == 0.F
            ? 0.5F
            : (waypoint_pos.x - args.start.x) / uv_axis.x;
        waypoint_pos.y = uv_axis.y == 0.F
            ? 0.5F
            : (waypoint_pos.y - args.start.y) / uv_axis.y;
        link.waypoints.emplace(
            link.waypoints.begin() + double_clicked,
            link_waypoint_t {
                .uv_pos = waypoint_pos,
            });
    }

    if (link.selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        link.marked_for_deletion = true;
    }
}

link_grid_path_t generate_path_from_node_to_pos(
    ImNodeFlow& ctx,
    ConstNodeRef node,
    ImVec2 pos)
{
    ImVec2 start = node.getTLPos() + node.getFullSize() * ImVec2(1.F, 0.F);
    ImVec2 end = pos;

    start.y += ImGui::GetTextLineHeight();

    auto p1 = start;
    auto p2 = start + ImVec2(20.F, 0.F);
    auto p3 = end - ImVec2(20.F, 0.F);
    auto p4 = end;

    return detail::generate_path(ctx, p1, p2, p3, p4);
}

link_grid_path_t generate_path_from_pin_to_pos(
    ImNodeFlow& ctx,
    ConstPinRef pin,
    ImVec2 pos)
{
    auto p1 = pin.pinPoint();
    auto p4 = pos;

    if (pin.getType() != PinType::Output) {
        std::swap(p1, p4);
    }

    auto p2 = p1 + ImVec2(20.F, 0.F);
    auto p3 = p4 - ImVec2(20.F, 0.F);

    return generate_path(ctx, p1, p2, p3, p4);
}

link_grid_path_t generate_path(
    ImNodeFlow& ctx,
    ImVec2 start_point_anchor,
    ImVec2 start_point,
    ImVec2 end_point,
    ImVec2 end_point_anchor,
    std::vector<link_waypoint_t> const& waypoints)
{
    link_grid_path_t result;
    result.points.emplace_back(link_grid_path_t::coord_t {
        .pos = start_point_anchor,
        .waypoint_index = 0,
    });
    result.points.emplace_back(link_grid_path_t::coord_t {
        .pos = start_point,
        .waypoint_index = 0,
    });

    ImVec2 uv_axis = end_point - start_point;
    for (int i = 0; i < waypoints.size(); i++) {
        const auto& waypoint = waypoints[i];
        ImVec2 waypoint_pos = start_point + waypoint.uv_pos * uv_axis;
        waypoint_pos = ctx.slamPosition(waypoint_pos);

        process_next_point(
            result,
            link_grid_path_t::coord_t {
                .pos = waypoint_pos,
                .waypoint_index = i,
            });
    }

    process_next_point(
        result,
        link_grid_path_t::coord_t {
            .pos = end_point,
            .waypoint_index = int(waypoints.size()),
        });
    process_next_point(
        result,
        link_grid_path_t::coord_t {
            .pos = end_point_anchor,
            .waypoint_index = int(waypoints.size()),
        });

    return result;
}

bool draw_path(
    ImNodeFlow& ctx,
    ImDrawList* draw_list,
    ImGuiID path_id,
    link_grid_path_t const& path,
    ImU32 start_color,
    ImU32 end_color,
    float thickness,
    bool selected,
    bool hovered,
    bool* out_hovered,
    int* out_double_clicked)
{
    ImGui::PushID(path_id);

    bool pressed = false;
    bool new_hovered = false;
    int new_double_clicked = int(-1);
    for (int i = 0; i < path.points.size() - 1; i++) {
        cubic_bezier_controls_t segment {
            .p1 = ctx.grid2screen(path.points[i].pos),
            .p2 = ctx.grid2screen(path.points[i].pos),
            .p3 = ctx.grid2screen(path.points[i + 1].pos),
            .p4 = ctx.grid2screen(path.points[i + 1].pos),
        };

        bool segment_hovered = false;
        bool segment_double_clicked = false;
        pressed |= detail::smart_bezier_widget(
            ImGui::GetID(i),
            draw_list,
            segment,
            start_color,
            end_color,
            selected,
            hovered,
            &segment_hovered,
            &segment_double_clicked,
            thickness,
            ctx.getStyle().link_gradient_blend);
        new_hovered |= segment_hovered;
        if (segment_double_clicked) {
            new_double_clicked = path.points[i + 1].waypoint_index;
        }
    }

    ImGui::PopID();

    if (out_hovered != nullptr) {
        *out_hovered = new_hovered;
    }
    if (out_double_clicked != nullptr) {
        *out_double_clicked = new_double_clicked;
    }

    return pressed;
}

} // namespace ImFlow::detail
