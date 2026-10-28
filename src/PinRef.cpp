#include "ImNodeFlow/PinRef.h"

#include "ImNodeFlow/Context.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/detail/IPin.h"

#include "./PinConnectionHolder.h"
#include "./pin_base.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cassert>

namespace ImFlow {

namespace {

    ImRect
    getSocketHitBounds(detail::pin_base_t const& pin, float expand_radius = -1.0F)
    {
        if (expand_radius < 0.0f) {
            expand_radius = pin.style.socket_hovered_radius;
        }

        ImVec2 center = PinRef<true> { pin }.pinPoint();
        ImVec2 tl = center - ImVec2(expand_radius, expand_radius);
        ImVec2 br = center + ImVec2(expand_radius, expand_radius);

        return { tl, br };
    }

    bool drawSocket(
        detail::pin_base_t const& pin,
        bool* hovered = nullptr,
        ImVec2* socketPos = nullptr)
    {
        float text_height = ImGui::GetTextLineHeight();
        float radius = pin.style.socket_radius;
        float maxRadius = std::max(
            pin.style.socket_radius,
            std::max(
                pin.style.socket_hovered_radius,
                pin.style.socket_connected_radius));
        ImU32 color = pin.style.color;
        int shape = pin.style.socket_shape;
        float thickness = pin.style.socket_thickness;
        bool circle_filled = false;

        if (PinRef<true> { pin }.isConnected()) {
            circle_filled = true;
            radius = pin.style.socket_connected_radius;
        } else if (pin.isHovered) {
            radius = pin.style.socket_hovered_radius;
        }

        ImVec2 socket_size { maxRadius * 2.0F, maxRadius * 2.0F };
        ImVec2 item_size { maxRadius * 2.0F, text_height };
        ImVec2 point = ImGui::GetCursorPos() + item_size / 2.0F;

        // advance cursor
        ImGui::ItemSize(item_size);

        ImRect bb {
            point - socket_size / 2.0F,
            point + socket_size / 2.0F,
        };

        bool is_hovered = false;
        bool is_pressed = false;

        if (ImGui::ItemAdd(bb, ImGui::GetID("##pinSocket"))) {
            is_pressed = ImGui::ButtonBehavior(
                bb,
                ImGui::GetID("##pinSocket"),
                &is_hovered,
                nullptr,
                ImGuiButtonFlags_PressedOnClick);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (circle_filled) {
                draw_list->AddCircleFilled(point, radius, color, shape);
            } else {
                draw_list->AddCircle(point, radius, color, shape, thickness);
            }
        }

        if (hovered != nullptr) {
            *hovered = is_hovered;
        }
        if (socketPos != nullptr) {
            *socketPos = point;
        }

        return is_pressed;
    }

    void
    drawDecoration(detail::pin_base_t const& pin)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        if (pin.isHovered) {
            draw_list->AddRectFilled(
                pin.pos - pin.style.extra.padding,
                pin.pos + pin.size + pin.style.extra.padding,
                pin.style.extra.bg_hover_color,
                pin.style.extra.bg_radius);
        } else {
            draw_list->AddRectFilled(
                pin.pos - pin.style.extra.padding,
                pin.pos + pin.size + pin.style.extra.padding,
                pin.style.extra.bg_color,
                pin.style.extra.bg_radius);
        }

        draw_list->AddRect(
            pin.pos - pin.style.extra.padding,
            pin.pos + pin.size + pin.style.extra.padding,
            pin.style.extra.border_color,
            pin.style.extra.bg_radius,
            0,
            pin.style.extra.border_thickness);
    }

} // anonymous namespace

template <bool TIsConst>
void PinRef<TIsConst>::update()
    requires(!TIsConst)
{
    detail::pin_base_t& pin = m_pin.get();

    ImGui::PushID(&pin);

    bool socket_hovered = false;
    bool socket_pressed = false;

    if (this->getType() == PinType::Output) {
        this->setPos(ImGui::GetCursorPos());
        ImGui::BeginGroup();
        pin.renderer(*this);
        ImGui::EndGroup();
        pin.size = ImGui::GetItemRectSize();
        drawDecoration(pin);
        ImGui::SameLine(0.f, 0.f);
        socket_pressed = drawSocket(m_pin, &socket_hovered, &pin.pinPoint);
    } else if (this->getType() == PinType::Input) {
        socket_pressed = drawSocket(m_pin, &socket_hovered, &pin.pinPoint);
        ImGui::SameLine(0.f, 0.f);
        this->setPos(ImGui::GetCursorPos());
        ImGui::BeginGroup();
        pin.renderer(*this);
        ImGui::EndGroup();
        pin.size = ImGui::GetItemRectSize();
        drawDecoration(pin);
    }

    // here, decoration is everything except the pin's socket
    bool decoration_hovered = false;
    bool decoration_pressed = false;

    ImRect decoration_bb { pin.pos, pin.pos + pin.size };
    ImGuiID decoration_id = ImGui::GetID("##pinDecor");
    if (ImGui::ItemAdd(decoration_bb, decoration_id)) {
        decoration_pressed = ImGui::ButtonBehavior(
            ImRect { pin.pos, pin.pos + pin.size },
            decoration_id,
            &decoration_hovered,
            nullptr,
            ImGuiButtonFlags_PressedOnClick);
    }

    bool is_hovering = decoration_hovered || socket_hovered;
    bool is_pressed = decoration_pressed || socket_pressed;

    ImNodeFlow& flow = pin.ctx.get();

    if (is_hovering) {
        flow.hovering(&pin);
        if (is_pressed) {
            flow.dragging(&pin);
        }
    }

    pin.isHovered = is_hovering;

    ImGui::PopID();
}

template <bool TIsConst>
PinRef<TIsConst>&
PinRef<TIsConst>::renderer(pin_rendering_handler_t r)
    requires(!TIsConst)
{
    auto& base = m_pin.get();
    base.renderer = std::move(r);
    return *this;
}

template <bool TIsConst>
void PinRef<TIsConst>::linkTo(PinRef<TIsConst> other)
    requires(!TIsConst)
{
    auto& base = m_pin.get();
    auto& other_base = other.m_pin.get();

    if (&base.ctx.get() != &other_base.ctx.get()) {
        // Cannot link pins across different contexts
        return;
    }

    auto& ctx = base.ctx.get();
    ctx.createLink(*this, other);
}

template <bool TIsConst>
void PinRef<TIsConst>::deleteAllLinks()
    requires(!TIsConst)
{
    auto& base = m_pin.get();
    auto links = std::move(base.links);
}

template <bool TIsConst>
void PinRef<TIsConst>::addLink(detail::Link* link)
    requires(!TIsConst)
{
    auto& base = m_pin.get();

    auto iter = std::find(base.links.begin(), base.links.end(), link);

    if (iter != base.links.end()) {
        return;
    }

    if (base.strat == PinConnStrat::AllowOnlySingleConnection) {
        this->deleteAllLinks();
    }

    base.links.emplace_back(link);
}

template <bool TIsConst>
void PinRef<TIsConst>::removeLink(detail::Link* link)
    requires(!TIsConst)
{
    auto& base = m_pin.get();

    auto iter = std::find_if(
        base.links.begin(),
        base.links.end(),
        [link](const auto& internal_link) -> bool {
            return internal_link == link;
        });

    if (iter != base.links.end()) {
        base.links.erase(iter);
    }
}

template <bool TIsConst>
bool PinRef<TIsConst>::isConnected() const
{
    auto& base = m_pin.get();
    return !base.links.empty();
}

template <bool TIsConst>
std::vector<PinRef<TIsConst>>
PinRef<TIsConst>::getConnections() const
{
    std::vector<PinRef<TIsConst>> result;

    detail::pin_base_t const& base = this->m_pin.get();
    result.reserve(base.links.size());

    for (const detail::PinConnectionHolder& connection : base.links) {
        auto* link = connection.link();

        // Not connected for some reason?
        if (link == nullptr) {
            continue;
        }

        switch (base.type) {
        case PinType::Input:
            result.emplace_back(link->outPin());
            break;

        case PinType::Output:
            result.emplace_back(link->inPin());
            break;

        default:
            assert(false && "Detected memory corruption");
            break;
        }
    }

    return result;
}

template <bool TIsConst>
PinUID PinRef<TIsConst>::getUid() const
{
    return m_pin.get().uid;
}

template <bool TIsConst>
const std::string& PinRef<TIsConst>::getName() const
{
    return m_pin.get().name;
}

template <bool TIsConst>
const ImVec2& PinRef<TIsConst>::getPos() const
{
    return m_pin.get().pos;
}

template <bool TIsConst>
const ImVec2& PinRef<TIsConst>::getSize() const
{
    return m_pin.get().size;
}

template <bool TIsConst>
NodeRefImpl<TIsConst> PinRef<TIsConst>::getParent() const
{
    return NodeRefImpl<TIsConst>(*m_pin.get().parent);
}

template <bool TIsConst>
PinType PinRef<TIsConst>::getType() const
{
    return m_pin.get().type;
}

template <bool TIsConst>
uint64_t PinRef<TIsConst>::getTypeID() const
{
    return m_pin.get().underlying_pin->getTypeID();
}

template <bool TIsConst>
const PinStyle& PinRef<TIsConst>::getStyle() const
{
    return m_pin.get().style;
}

template <bool TIsConst>
void PinRef<TIsConst>::setStyle(PinStyle style)
    requires(!TIsConst)
{
    m_pin.get().style = std::move(style);
}

template <bool TIsConst>
void PinRef<TIsConst>::allowSameNodeConnections(bool state)
    requires(!TIsConst)
{
    m_pin.get().allowSelfConnection = state;
}

template <bool TIsConst>
bool PinRef<TIsConst>::allowsSameNodeConnections() const
{
    return m_pin.get().allowSelfConnection;
}

template <bool TIsConst>
ImVec2 PinRef<TIsConst>::pinPoint() const
{
    auto& base = m_pin.get();

    if (base.type == PinType::Input) {
        return base.pos + ImVec2(-base.style.extra.socket_padding, base.size.y / 2);
    }
    return base.pos + ImVec2(base.size.x + base.style.extra.socket_padding, base.size.y / 2);
}

template <bool TIsConst>
float PinRef<TIsConst>::calcWidth() const
{
    return ImGui::CalcTextSize(m_pin.get().name.c_str()).x;
}

template <bool TIsConst>
void PinRef<TIsConst>::setPos(ImVec2 pos)
    requires(!TIsConst)
{
    m_pin.get().pos = pos;
}

template <bool TIsConst>
void PinRef<TIsConst>::setSocketHitboxEnabled(bool enabled)
    requires(!TIsConst)
{
    m_pin.get().socketHitboxEnabled = enabled;
}

template <bool TIsConst>
void PinRef<TIsConst>::calculate()
    requires(!TIsConst)
{
    m_pin.get().underlying_pin->calculate(PinRef { m_pin.get() });
}

template <bool TIsConst>
void PinRef<TIsConst>::prime()
    requires(!TIsConst)
{
    m_pin.get().underlying_pin->prime();
}

template <bool TIsConst>
detail::IPin&
PinRef<TIsConst>::getDetail() const
    requires(!TIsConst)
{
    return *m_pin.get().underlying_pin;
}

template <bool TIsConst>
detail::IPin const&
PinRef<TIsConst>::getDetail() const
{
    return *m_pin.get().underlying_pin;
}

template class PinRef<false>;
template class PinRef<true>;

} // namespace ImFlow
