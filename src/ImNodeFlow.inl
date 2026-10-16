#pragma once

#include "ImNodeFlow.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>

namespace ImFlow {

namespace detail {

    inline void
    smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x)
            delta += 0.2f * (p1.x - p2.x);
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        if (p2.x < p1.x - 50.f)
            delta *= -1.f;
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        dl->AddBezierCubic(p1, p11, p22, p2, color, thickness);
    }

    inline bool
    smart_bezier_collider(
        const ImVec2& p,
        const ImVec2& p1,
        const ImVec2& p2,
        float radius)
    {
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x)
            delta += 0.2f * (p1.x - p2.x);
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        if (p2.x < p1.x - 50.f)
            delta *= -1.f;
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        return ImProjectOnCubicBezier(p, p1, p11, p22, p2).Distance < radius;
    }

    inline uint64_t
    next_type_id()
    {
        static uint64_t id = 0;
        return ++id;
    }

    template <typename T>
    inline uint64_t type_id = next_type_id();

    template <typename T>
    inline static uint64_t
    get_type_id()
    {
        return type_id<T>;
    }

} // namespace detail

// -----------------------------------------------------------------------------------------------------------------
// FLOW CONTEXT

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNode(const ImVec2& pos, Params&&... args)
{
    return addNode(
        pos,
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNodeAt(const ImVec2& pos, Params&&... args)
{
    return placeNodeAt(
        pos, std::make_unique<T>(std::forward<Params>(args)...));
}

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNodeOnMouse(Params&&... args)
{
    return placeNode(std::make_unique<T>(std::forward<Params>(args)...));
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

template <typename T>
std::shared_ptr<InPin<T>>
Node::addIN(InPinInfo<T> pin_info)
{
    return addIN(PinUID::generate(), std::move(pin_info));
}

template <typename T>
std::shared_ptr<InPin<T>>
Node::addIN(const PinUID& uid, InPinInfo<T> pin_info)
{
    auto p = std::make_shared<InPin<T>>(
        m_ctx, uid, this, std::move(pin_info));
    m_ins.emplace_back(p);
    return p;
}

template <typename T>
const T&
Node::showIN(InPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_dynamicIns) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return dyn_pin.pin().toIn<T>().val();
        }
    }

    m_dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(
            m_ctx, PinUID::generate(), this, std::move(pin_info)));
    return m_dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
const T&
Node::showIN(const PinUID& uid, InPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_dynamicIns) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return dyn_pin.pin().toIn<T>().val();
        }
    }

    m_dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(m_ctx, uid, this, std::move(pin_info)));
    return m_dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
std::shared_ptr<OutPin<T>>
Node::addOUT(OutPinInfo<T> pin_info)
{
    return addOUT<T>(PinUID::generate(), std::move(pin_info));
}

template <typename T>
std::shared_ptr<OutPin<T>>
Node::addOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    auto p = std::make_shared<OutPin<T>>(
        m_ctx, uid, this, std::move(pin_info));
    m_outs.emplace_back(p);
    return p;
}

template <typename T>
void Node::showOUT(OutPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_dynamicOuts) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_ctx, PinUID::generate(), this, std::move(pin_info)));
}

template <typename T>
void Node::showOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_dynamicOuts) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_ctx, uid, this, std::move(pin_info)));
    m_dynamicOuts.back().pin().toOut<T>().behaviour(
        std::move(pin_info.behaviour));
}

template <typename T>
InPin<T>*
Node::inPin(const PinUID& uid)
{
    auto* pin = this->inPin(uid);
    return pin ? &pin->toIn<T>() : nullptr;
}

template <typename T>
InPin<T>*
Node::inPin(const char* name)
{
    auto* pin = this->inPin(name);
    return pin ? &pin->toIn<T>() : nullptr;
}

template <typename T>
OutPin<T>*
Node::outPin(const PinUID& uid)
{
    auto* pin = this->outPin(uid);
    return pin ? &pin->toOut<T>() : nullptr;
}

template <typename T>
OutPin<T>*
Node::outPin(const char* name)
{
    auto* pin = this->outPin(name);
    return pin ? &pin->toOut<T>() : nullptr;
}

// -----------------------------------------------------------------------------------------------------------------
// PIN

inline void
Pin::drawSocket()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 tl = pinPoint() - ImVec2(m_style.socket_radius, m_style.socket_radius);
    ImVec2 br = pinPoint() + ImVec2(m_style.socket_radius, m_style.socket_radius);

    if (isConnected())
        draw_list->AddCircleFilled(
            pinPoint(),
            m_style.socket_connected_radius,
            m_style.color,
            m_style.socket_shape);
    else {
        if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
            draw_list->AddCircle(
                pinPoint(),
                m_style.socket_hovered_radius,
                m_style.color,
                m_style.socket_shape,
                m_style.socket_thickness);
        else
            draw_list->AddCircle(
                pinPoint(),
                m_style.socket_radius,
                m_style.color,
                m_style.socket_shape,
                m_style.socket_thickness);
    }

    if (ImGui::IsMouseHoveringRect(tl, br)) {
        m_ctx.get().hovering(this);
    }
}

inline void
Pin::drawDecoration()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (ImGui::IsItemHovered())
        draw_list->AddRectFilled(
            m_pos - m_style.extra.padding,
            m_pos + m_size + m_style.extra.padding,
            m_style.extra.bg_hover_color,
            m_style.extra.bg_radius);
    else
        draw_list->AddRectFilled(
            m_pos - m_style.extra.padding,
            m_pos + m_size + m_style.extra.padding,
            m_style.extra.bg_color,
            m_style.extra.bg_radius);
    draw_list->AddRect(
        m_pos - m_style.extra.padding,
        m_pos + m_size + m_style.extra.padding,
        m_style.extra.border_color,
        m_style.extra.bg_radius,
        0,
        m_style.extra.border_thickness);
}

inline void
Pin::update()
{
    if (!m_renderer) {
        ImGui::SetCursorPos(m_pos);
        ImGui::Text("%s", m_name.c_str());
    } else {
        ImGui::BeginGroup();
        m_renderer(this);
        ImGui::EndGroup();
    }

    m_size = ImGui::GetItemRectSize();

    if (!m_renderer) {
        drawDecoration();
        drawSocket();
    }

    if (ImGui::IsItemHovered()) {
        auto& ctx = m_ctx.get();

        ctx.hovering(this);
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            ctx.dragging(this);
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------
// PIN

template <typename T>
InPin<T>&
Pin::toIn()
{
    assert(this->m_type == PinType::PinType_Input && this->getTypeID() == detail::type_id<T> && "Invalid cast attempted");
    return static_cast<InPin<T>&>(*this);
}

template <typename T>
InPin<T> const&
Pin::toIn() const
{
    assert(this->m_type == PinType::PinType_Input && this->getTypeID() == detail::type_id<T> && "Invalid cast attempted");
    return static_cast<InPin<T>&>(*this);
}

template <typename T>
OutPin<T>&
Pin::toOut()
{
    assert(this->m_type == PinType::PinType_Output && this->getTypeID() == detail::type_id<T> && "Invalid cast attempted");
    return static_cast<OutPin<T>&>(*this);
}

template <typename T>
OutPin<T> const&
Pin::toOut() const
{
    assert(this->m_type == PinType::PinType_Output && this->getTypeID() == detail::type_id<T> && "Invalid cast attempted");
    return static_cast<OutPin<T>&>(*this);
}

// -----------------------------------------------------------------------------------------------------------------
// IN PIN

template <class T>
void InPin<T>::calculate()
{
    m_valNext = m_emptyVal;

    bool first = true;
    for (auto& link : m_links) {
        m_valNext = m_accumulatorOp(
            first,
            m_valNext,
            link->outPin().template toOut<T>().value());
        first = false;
    }
}

template <class T>
void InPin<T>::prime()
{
    m_val = m_valNext;
}

template <class T>
const T&
InPin<T>::value()
{
    return m_val;
}

// -----------------------------------------------------------------------------------------------------------------
// OUT PIN

template <class T>
void OutPin<T>::calculate()
{
    m_valNext = m_behaviour(*getParent());
}

template <class T>
void OutPin<T>::prime()
{
    m_val = m_valNext;
}

template <class T>
const T&
OutPin<T>::value()
{
    return m_val;
}

} // namespace ImFlow
