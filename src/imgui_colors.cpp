#include "imgui_colors.h"

namespace ImFlow::detail {

ImU32 get_selected_color(double t)
{
    float v = std::sin(t) * 0.1F + 0.85F;
    ImVec4 rgb { v, v, v, 1.F };
    return ImGui::ColorConvertFloat4ToU32(rgb);
}

} // namespace ImFlow::detail
