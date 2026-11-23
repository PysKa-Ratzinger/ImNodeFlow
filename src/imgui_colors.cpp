#include "imgui_colors.h"

#include <cmath>

namespace ImFlow::detail {

ImU32 get_selected_color(double t)
{
#if __cplusplus >= 202600L
    float v = std::sinf(t) * 0.1F + 0.85F;
#else
    float v = std::sin(t) * 0.1F + 0.85F;
#endif
    ImVec4 rgb { v, v, v, 1.F };
    return ImGui::ColorConvertFloat4ToU32(rgb);
}

} // namespace ImFlow::detail
