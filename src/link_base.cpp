#include "link_base.h"

#include "ImNodeFlow/Context.h"
#include "ImNodeFlow/fwd.h"

#include "./editor_context.h"
#include "./imgui_bezier_items.h"
#include "./imgui_bezier_widgets.h"
#include "./imgui_colors.h"
#include "./imgui_custom_widgets.h"

#include "imgui.h"
#include "imgui_extra_math.h"
#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <vector>

namespace ImFlow::detail {

void link_base_update(
    ImNodeFlow& ctx,
    link_base_t& link,
    link_base_update_args args)
{
    ImVec2 uv_axis = args.end - args.start;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    bool is_ctrl_held_down = ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
        || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

    ImDrawListSplitter splitter;
    if (!link.waypoints.empty()) {
        splitter.Split(draw_list, 2);
        splitter.SetCurrentChannel(draw_list, 1);
    }

    ImGui::PushID("##waypoints");
    for (size_t i = 0; i < link.waypoints.size(); i++) {
        auto& waypoint = link.waypoints[i];

        bool is_selected = waypoint.selectable.isSelected(ctx.getDetail().selectableCtx);

        if (is_selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            waypoint.marked_for_deletion = true;
            continue;
        }

        bool is_held = false;
        bool is_hovered = false;
        bool is_released = false;
        bool is_clicked = drag_pin_widget(
            ImGui::GetID(i),
            draw_list,
            ctx.grid2screen(waypoint.grid_pos),
            6.F,
            waypoint.held,
            is_selected,
            &is_hovered,
            &is_held,
            &is_released);
        waypoint.held = is_held;

        is_clicked = is_clicked && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (is_clicked) {
            ctx.startNodeDrag();
        } else if (is_released) {
            ctx.stopNodeDrag();
        }

        bool wants_drag = is_selected || args.drag_everything;
        bool was_dragged = false;
        if (wants_drag) {
            ImVec2 drag_vec;
            bool should_apply_drag = ctx.getNodeDragAmmount(&drag_vec);

            if (!waypoint.dragging) {
                waypoint.grid_pos_backup = waypoint.grid_pos;
                waypoint.dragging = true;
            }

            waypoint.grid_pos = waypoint.grid_pos_backup + drag_vec;
            was_dragged = drag_vec != ImVec2(0.F, 0.F);

            waypoint.grid_pos = ctx.slamPosition(waypoint.grid_pos);

            if (should_apply_drag) {
                waypoint.dragging = false;
            }
        }

        waypoint.selectable.update(
            ctx.getDetail().selectableCtx,
            is_ctrl_held_down,
            is_clicked,
            is_released,
            was_dragged);
    }
    ImGui::PopID();

    auto iter = std::remove_if(
        std::begin(link.waypoints),
        std::end(link.waypoints),
        [](link_waypoint_t const& waypoint) -> bool {
            return waypoint.marked_for_deletion;
        });
    if (iter != link.waypoints.end()) {
        link.waypoints.erase(iter, link.waypoints.end());
    }

    auto path = detail::generate_path(
        ctx,
        args.start_anchor,
        args.start,
        args.end,
        args.end_anchor,
        link.waypoints);

    if (!link.waypoints.empty()) {
        splitter.SetCurrentChannel(draw_list, 0);
    }

    int double_clicked = -1;
    bool is_released = false;
    bool clicked = draw_path(
        ctx,
        draw_list,
        ImGui::GetID(&link),
        path,
        args.start_color,
        args.end_color,
        args.thickness,
        link.selectable.isSelected(ctx.getDetail().selectableCtx),
        link.hovered,
        &link.hovered,
        &double_clicked,
        &is_released);

    splitter.Merge(draw_list);

    link.selectable.update(
        ctx.getDetail().selectableCtx,
        is_ctrl_held_down,
        clicked,
        is_released,
        false);

    if (args.draw_arrow != 0.0F && args.end != args.end_anchor) {
        ImVec2 v = ctx.grid2screen(args.end_anchor) - ctx.grid2screen(args.end);
        v = ImNormalized(v) * 10.0F;
        ImVec2 v_r90 { v.y, -v.x };
        float a = args.draw_arrow;
        auto arrow_b = ctx.grid2screen(args.end_anchor) - v * a;

        std::array<ImVec2, 3> arrow_points;
        arrow_points[0] = arrow_b - v_r90 * a;
        arrow_points[1] = ctx.grid2screen(args.end_anchor);
        arrow_points[2] = arrow_b + v_r90 * a;

        auto arrow_color = args.end_color;
        if (link.selectable.isSelected(ctx.getDetail().selectableCtx)) {
            arrow_color = detail::get_selected_color((ImGui::GetTime() * 2.0F) + 2.0F);
        }

        draw_list->AddPolyline(
            arrow_points.data(),
            arrow_points.size(),
            arrow_color,
            args.thickness);
    }

    if (double_clicked >= 0 && double_clicked <= link.waypoints.size()) {
        ImVec2 waypoint_pos = ctx.screen2grid(ImGui::GetMousePos());
        waypoint_pos = ctx.slamPosition(waypoint_pos);
        link.waypoints.emplace(
            link.waypoints.begin() + double_clicked,
            link_waypoint_t {
                .grid_pos = waypoint_pos,
                .selectable { ctx.getDetail().selectableCtx },
            });
    }

    if (link.selectable.isSelected(ctx.getDetail().selectableCtx)
        && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        link.marked_for_deletion = true;
    }
}

void link_base_prepare_next_frame(ImNodeFlow& ctx, link_base_t& link)
{
    link.selectable.prepareNextFrame(ctx.getDetail().selectableCtx);
    for (auto& waypoint : link.waypoints) {
        waypoint.selectable.prepareNextFrame(ctx.getDetail().selectableCtx);
    }
}

link_grid_path_t generate_path_from_node_to_pos(
    ImNodeFlow& ctx,
    ConstNodeRef node,
    ImVec2 grid_pos)
{
    ImVec2 start = node.getTLPos() + node.getFullSize() * ImVec2(1.F, 0.F);
    ImVec2 end = grid_pos;

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
    ImVec2 grid_pos)
{
    auto p1 = pin.pinPoint();
    auto p4 = grid_pos;

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
        .grid_pos = start_point_anchor,
        .waypoint_index = 0,
    });
    result.points.emplace_back(link_grid_path_t::coord_t {
        .grid_pos = start_point,
        .waypoint_index = 0,
    });

    ImVec2 uv_axis = end_point - start_point;
    for (int i = 0; i < waypoints.size(); i++) {
        const auto& waypoint = waypoints[i];

        result.points.emplace_back(
            link_grid_path_t::coord_t {
                .grid_pos = waypoint.grid_pos,
                .waypoint_index = i,
            });
    }

    result.points.emplace_back(
        link_grid_path_t::coord_t {
            .grid_pos = end_point,
            .waypoint_index = int(waypoints.size()),
        });
    result.points.emplace_back(
        link_grid_path_t::coord_t {
            .grid_pos = end_point_anchor,
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
    int* out_double_clicked,
    bool* out_released)
{
    ImGui::PushID(path_id);

    bool pressed = false;
    bool new_hovered = false;
    bool is_released = false;
    int new_double_clicked = int(-1);

    float total_length = 0;
    for (int i = 0; i < path.points.size() - 1; i++) {
        auto p1 = ctx.grid2screen(path.points[i].grid_pos);
        auto p4 = ctx.grid2screen(path.points[i + 1].grid_pos);
        ImVec2 v = p4 - p1;
        if (v == ImVec2(0.F, 0.F)) {
            continue;
        }
        total_length += ImSqrt(v.x * v.x + v.y * v.y);
    }

    ImVec4 c1 = ImGui::ColorConvertU32ToFloat4(start_color);
    ImVec4 c2 = ImGui::ColorConvertU32ToFloat4(end_color);

    ImVec2 previous_ctrl_pt { 0.F, 0.F };

    float cumm_l = 0.F;
    for (int i = 0; i < path.points.size() - 1; i++) {
        cubic_bezier_controls_t segment {
            .p1 = ctx.grid2screen(path.points[i].grid_pos),
            .p2 = ctx.grid2screen(path.points[i].grid_pos),
            .p3 = ctx.grid2screen(path.points[i + 1].grid_pos),
            .p4 = ctx.grid2screen(path.points[i + 1].grid_pos),
        };

        ImVec2 ctrl_pt { 0.F, 0.F };

        if (i < path.points.size() - 2) {
            auto prev = segment.p1;
            auto curr = segment.p4;
            auto next = ctx.grid2screen(path.points[i + 2].grid_pos);

            auto to_next = ImNormalized(next - curr);
            auto to_prev = ImNormalized(prev - curr);
            if (to_next == ImVec2(0.F, 0.F) && to_prev == ImVec2(0.F, 0.F)) {
                ctrl_pt = { 0.F, 0.F };
            } else if (to_next == ImVec2(0.F, 0.F)) {
                ctrl_pt = ImNormalized(to_prev);
            } else if (to_prev == ImVec2(0.F, 0.F)) {
                ctrl_pt = ImNormalized(to_next);
            } else {
                auto vec = to_next - to_prev;
                if (ImAbs(vec.x) < 0.001F && ImAbs(vec.y) < 0.001F) {
                    ctrl_pt = ImVec2 { to_next.y, -to_next.x };
                } else {
                    ctrl_pt = ImNormalized(to_next - to_prev);
                }
            }
        }

        ImVec2 v = segment.p4 - segment.p1;
        if (v == ImVec2(0.F, 0.F)) {
            previous_ctrl_pt = ctrl_pt;
            continue;
        }

        float l = ImSqrt(v.x * v.x + v.y * v.y);
        ImU32 seg_start_color = ImGui::ColorConvertFloat4ToU32(ImLerp(c1, c2, cumm_l / total_length));
        ImU32 seg_end_color = ImGui::ColorConvertFloat4ToU32(ImLerp(c1, c2, (cumm_l + l) / total_length));
        cumm_l += l;

        if (i != 0 && i != path.points.size() - 2) {
            ImVec2 prev_pt = ctx.grid2screen(path.points[i - 1].grid_pos);
            ImVec2 next_pt = ctx.grid2screen(path.points[i + 2].grid_pos);

            // First control point excepting the first segment
            if (i == 1) {
                auto v_prev = segment.p1 - prev_pt;
                if (v_prev != ImVec2(0.F, 0.F)) {
                    segment.p2 = segment.p1 + ImNormalized(v_prev) * l * 0.25;
                }
            } else {
                segment.p2 = segment.p1 + previous_ctrl_pt * l * 0.25;
            }

            if (i == path.points.size() - 3) {
                auto v_next = next_pt - segment.p4;
                if (v_next != ImVec2(0.F, 0.F)) {
                    segment.p3 = segment.p4 - ImNormalized(v_next) * l * 0.25;
                }
            } else {
                segment.p3 = segment.p4 - ctrl_pt * l * 0.25;
            }
        }

        previous_ctrl_pt = ctrl_pt;

        bool segment_hovered = false;
        bool segment_double_clicked = false;
        bool segment_released = false;
        pressed |= detail::smart_bezier_widget(
            ImGui::GetID(i),
            draw_list,
            segment,
            seg_start_color,
            seg_end_color,
            selected,
            hovered,
            &segment_hovered,
            &segment_double_clicked,
            &segment_released,
            thickness,
            ctx.getStyle().link_gradient_blend);
        new_hovered |= segment_hovered;
        is_released |= segment_released;
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
    if (out_released != nullptr) {
        *out_released = is_released;
    }

    return pressed;
}

} // namespace ImFlow::detail
