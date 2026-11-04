#include "pin_drawing_utils.h"

#include "ImNodeFlow/PinRef.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/fwd.h"

#include "./imgui_bezier_items.h"
#include "./pin_base.h"

#include "imgui.h"

#include <vector>

namespace ImFlow::detail {

void draw_bezier_from_pin_to_pos(
    ImDrawList* draw_list,
    ConstPinRef pin,
    ImVec2 pos)
{
    // Links drag-out
    std::vector<cubic_bezier_controls_t> path;
    if (pin.getType() == PinType::Output) {
        path = detail::smart_bezier_path_between_two_pins(pin.pinPoint(), pos);
    } else {
        path = detail::smart_bezier_path_between_two_pins(pos, pin.pinPoint());
    }

    for (auto& segment : path) {
        detail::smart_bezier(
            draw_list,
            segment,
            pin.getStyle().color,
            pin.getStyle().extra.link_dragged_thickness);
    }
}

} // namespace ImFlow::detail
