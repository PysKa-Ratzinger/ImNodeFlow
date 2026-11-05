#include "pin_drawing_utils.h"

#include "ImNodeFlow/fwd.h"

#include "./PinLinkRef.h"
#include "./link_base.h"

#include "imgui.h"

namespace ImFlow::detail {

void draw_path_from_pin_to_pos(
    ImDrawList* draw_list,
    ImNodeFlow& ctx,
    ConstPinRef pin,
    ImVec2 grid_pos)
{
    // Links drag-out
    link_grid_path_t path = generate_path_from_pin_to_pos(ctx, pin, grid_pos);

    draw_path(
        ctx,
        draw_list,
        0,
        path,
        pin.getStyle().color,
        pin.getStyle().color,
        pin.getStyle().extra.link_dragged_thickness);
}

} // namespace ImFlow::detail
