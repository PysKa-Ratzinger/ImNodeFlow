#include "PinLinkRef.h"

#include "ImNodeFlow/PinRef.h"

#include "./imgui_bezier_items.h"
#include "./pin_base.h"

#include "ImNodeFlow/PinType.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "pin_link.h"

#include <cassert>
#include <utility>

namespace ImFlow::detail {

template <bool TIsConst>
PinLinkRef<TIsConst>::PinLinkRef(context_type& ctx, link_type& link)
    : m_ctx(ctx)
    , m_link(link)
{
}

template <bool TIsConst>
PinRef<TIsConst> PinLinkRef<TIsConst>::outPin() const
{
    return PinRef<TIsConst> { m_ctx, *m_link.get().out_pin };
}

template <bool TIsConst>
PinRef<TIsConst> PinLinkRef<TIsConst>::inPin() const
{
    return PinRef<TIsConst> { m_ctx, *m_link.get().in_pin };
}

template <bool TIsConst>
bool PinLinkRef<TIsConst>::isAlive() const
{
    auto& link = m_link.get();
    return !link.marked_for_deletion
        && link.in_pin != nullptr
        && link.out_pin != nullptr;
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::connect(pin_base_t& out_pin, pin_base_t& in_pin)
    requires(!TIsConst)
{
    this->destroy();

    auto& link = m_link.get();
    link.marked_for_deletion = false;
    link.out_pin = &out_pin;
    link.in_pin = &in_pin;

    assert(link.out_pin != link.in_pin
        && "Links cannot be created starting and ending in the same node");
    assert(link.out_pin->type != link.in_pin->type
        && "Links cannot be created between pins of the same type");

    if (link.out_pin->type == PinType::Input) {
        std::swap(link.out_pin, link.in_pin);
    }

    this->outPin().addLink(&link);
    this->inPin().addLink(&link);
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::destroy()
    requires(!TIsConst)
{
    auto& link = m_link.get();
    if (link.out_pin != nullptr) {
        this->outPin().removeLink(&link);
        link.out_pin = nullptr;
    }
    if (link.in_pin != nullptr) {
        this->inPin().removeLink(&link);
        link.in_pin = nullptr;
    }
    link.marked_for_deletion = true;
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::update()
    requires(!TIsConst)
{
    detail::pin_link_t& link = m_link.get();

    if (link.marked_for_deletion) {
        return;
    }

    ImVec2 start = outPin().pinPoint();
    ImVec2 end = inPin().pinPoint();
    float thickness = outPin().getStyle().extra.link_thickness;

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        link.selected = false;
    }

    auto mouse_pos = ImGui::GetMousePos();
    link.hovered = detail::smart_bezier_collider(mouse_pos, start, end, 2.5);

    if (link.hovered) {
        ImRect always_valid_bb {
            mouse_pos - ImVec2(1.F, 1.F),
            mouse_pos + ImVec2(1.F, 1.F),
        };

        bool pressed = false;
        if (ImGui::ItemAdd(always_valid_bb, ImGui::GetID(this))) {
            pressed = ImGui::ButtonBehavior(
                always_valid_bb,
                ImGui::GetID(this),
                &link.hovered,
                nullptr,
                ImGuiButtonFlags_PressedOnClick);
        }

        link.selected = link.selected || pressed;
    }

    if (link.hovered) {
        thickness = outPin().getStyle().extra.link_hovered_thickness;
    }

    auto start_color = outPin().getStyle().color;
    auto end_color = inPin().getStyle().color;

    if (link.selected) {
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
        ImGui::GetWindowDrawList(),
        start,
        end,
        start_color,
        end_color,
        thickness,
        m_ctx.get().getStyle().link_gradient_blend);

    if (link.selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        this->destroy();
    }
}

template class PinLinkRef<false>;
template class PinLinkRef<true>;

} // namespace ImFlow::detail
