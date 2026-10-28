#include "./Link.h"

#include "ImNodeFlow/Context.h"
#include "ImNodeFlow/PinRef.h"

#include "./imgui_bezier_items.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace ImFlow::detail {

Link::Link(pin_base_t& out_pin, pin_base_t& in_pin) noexcept
    : m_out_pin(&out_pin)
    , m_in_pin(&in_pin)
{
    assert(m_out_pin != m_in_pin && "Links cannot be created starting and ending in the same node");

    PinRef<> { *m_out_pin }.addLink(this);
    PinRef<> { *m_in_pin }.addLink(this);
}

Link::~Link() noexcept
{
    this->destroy();
}

void Link::destroy() noexcept
{
    if (m_out_pin != nullptr) {
        PinRef<> { *m_out_pin }.removeLink(this);
        m_out_pin = nullptr;
    }
    if (m_in_pin != nullptr) {
        PinRef<> { *m_in_pin }.removeLink(this);
        m_in_pin = nullptr;
    }
    m_marked_for_deletion = true;
}

void Link::update(ImNodeFlow& ctx)
{
    PinRef<> in_pin { *m_in_pin };
    PinRef<> out_pin { *m_out_pin };

    ImVec2 start = out_pin.pinPoint();
    ImVec2 end = in_pin.pinPoint();
    float thickness = out_pin.getStyle().extra.link_thickness;

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_selected = false;
    }

    auto mouse_pos = ImGui::GetMousePos();
    m_hovered = detail::smart_bezier_collider(mouse_pos, start, end, 2.5);

    if (m_hovered) {
        ImRect always_valid_bb {
            mouse_pos - ImVec2(1.F, 1.F),
            mouse_pos + ImVec2(1.F, 1.F),
        };

        bool pressed = ImGui::ButtonBehavior(
            always_valid_bb,
            ImGui::GetID(this),
            &m_hovered,
            nullptr,
            ImGuiButtonFlags_PressedOnClick);

        m_selected = m_selected || pressed;
    }

    if (m_hovered) {
        thickness = out_pin.getStyle().extra.link_hovered_thickness;
    }

    auto start_color = out_pin.getStyle().color;
    auto end_color = in_pin.getStyle().color;

    if (m_selected) {
        double t = ImGui::GetTime();

        float v0 = std::sin(t * 2.F) * 0.1F + 0.85F;
        float v1 = std::sin(t * 2.F + 2.F) * 0.1F + 0.85F;

        ImVec4 start_rgb {
            v0,
            v0,
            v0,
            1.F,
        };
        ImVec4 end_rgb {
            v1,
            v1,
            v1,
            1.F,
        };

        start_color = ImGui::ColorConvertFloat4ToU32(start_rgb);
        end_color = ImGui::ColorConvertFloat4ToU32(end_rgb);
    }

    smart_bezier_gradient(
        start,
        end,
        start_color,
        end_color,
        thickness,
        ctx.getStyle().link_gradient_blend);

    if (m_selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        this->destroy();
    }
}

} // namespace ImFlow::detail
