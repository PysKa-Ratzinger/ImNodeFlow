#pragma once

#include "ImNodeFlow.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace ImFlow {

namespace detail {

    inline void
    smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x) {
            delta += 0.2f * (p1.x - p2.x);
            delta = std::min(150.F, delta);
        }
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        dl->AddBezierCubic(p1, p11, p22, p2, color, thickness);
    }

    inline void
    smart_bezier_gradient(
        const ImVec2& p1,
        const ImVec2& p2,
        ImU32 color1,
        ImU32 color2,
        float thickness,
        float blend)
    {
        if (color1 == color2) {
            smart_bezier(p1, p2, color1, thickness);
            return;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x) {
            delta += 0.2f * (p1.x - p2.x);
            delta = std::min(150.F, delta);
        }
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        ImVec2 p11 = p1 + ImVec2(delta, vert);

        const int segments = 32;
        ImVec4 c1 = ImGui::ColorConvertU32ToFloat4(color1);
        ImVec4 c2 = ImGui::ColorConvertU32ToFloat4(color2);
        ImVec2 prev = p1;
        for (int i = 1; i <= segments; ++i) {
            float t = (float)i / (float)segments;
            float u = 1.f - t;
            ImVec2 pt(
                u * u * u * p1.x + 3 * u * u * t * p11.x + 3 * u * t * t * p22.x + t * t * t * p2.x,
                u * u * u * p1.y + 3 * u * u * t * p11.y + 3 * u * t * t * p22.y + t * t * t * p2.y);
            float tm = ((float)i - 0.5f) / (float)segments;
            if (blend != 1.f) // symmetric contrast curve around the midpoint
                tm = (tm < 0.5f) ? 0.5f * powf(2.f * tm, blend) : 1.f - 0.5f * powf(2.f * (1.f - tm), blend);
            ImU32 col = ImGui::ColorConvertFloat4ToU32(ImLerp(c1, c2, tm));
            dl->AddLine(prev, pt, col, thickness);
            prev = pt;
        }
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
        if (p2.x < p1.x) {
            delta += 0.2f * (p1.x - p2.x);
            delta = std::min(150.F, delta);
        }
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
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
// NODE FLOW CONTEXT

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNode(const ImVec2& grid_coords, Params&&... params)
{
    return this->addNode(
        grid_coords,
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(params)...),
        });
}

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNodeAt(const ImVec2& screen_coords, Params&&... params)
{
    return this->emplaceNode(
        screen2grid(screen_coords), std::forward<Params>(params)...);
}

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNodeOnMouse(Params&&... params)
{
    return this->emplaceNode(
        ImGui::GetMousePos(), std::forward<Params>(params)...);
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

template <typename T>
Pin& Node::addIN(InPinInfo<T> pin_info)
{
    // TODO: Make user UID is unique (or disallow the use of user provided UIDs)
    return addIN(PinUID::generate(), std::move(pin_info));
}

template <typename T>
Pin& Node::addIN(const PinUID& uid, InPinInfo<T> pin_info)
{
    m_ins.emplace_back(
        std::make_unique<InPin<T>>(
            m_flow, *this, uid, std::move(pin_info)));
    return *m_ins.back();
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
            m_flow, *this, PinUID::generate(), std::move(pin_info)));
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
        std::make_unique<InPin<T>>(
            m_flow, *this, uid, std::move(pin_info)));
    return m_dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
Pin& Node::addOUT(OutPinInfo<T> pin_info)
{
    // TODO: Make user UID is unique (or disallow the use of user provided UIDs)
    return addOUT<T>(PinUID::generate(), std::move(pin_info));
}

template <typename T>
Pin& Node::addOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    m_outs.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow, *this, uid, std::move(pin_info)));
    return *m_outs.back();
}

template <typename T>
void Node::showOUT(OutPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_dynamicOuts) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            m_dynamicOuts.back().pin().toOut<T>().setBehaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow, *this, PinUID::generate(), std::move(pin_info)));
}

template <typename T>
void Node::showOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_dynamicOuts) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            m_dynamicOuts.back().pin().toOut<T>().setBehaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow, *this, uid, std::move(pin_info)));
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
    auto* pin = this->inPin(uid);
    return pin ? &pin->toIn<T>() : nullptr;
}

template <typename T>
OutPin<T>*
Node::outPin(const char* name)
{
    auto* pin = this->inPin(name);
    return pin ? &pin->toIn<T>() : nullptr;
}

// -----------------------------------------------------------------------------------------------------------------
// PIN

template <class T>
InPin<T>&
Pin::toIn()
{
    // check if casting is safe without RTTI
    assert(m_kind == PinType_Input && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<InPin<T>&>(*this);
}

template <class T>
InPin<T> const&
Pin::toIn() const
{
    // check if casting is safe without RTTI
    assert(m_kind == PinType_Input && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<InPin<T> const&>(*this);
}

template <class T>
OutPin<T>&
Pin::toOut()
{
    // check if casting is safe without RTTI
    assert(m_kind == PinType_Output && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<OutPin<T>&>(*this);
}

template <class T>
OutPin<T> const&
Pin::toOut() const
{
    // check if casting is safe without RTTI
    assert(m_kind == PinType_Output && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<OutPin<T> const&>(*this);
}

// -----------------------------------------------------------------------------------------------------------------
// IN PIN

template <class T>
InPin<T>::~InPin() = default;

template <class T>
void InPin<T>::calculate([[maybe_unused]] Node& node)
{
    m_valNext = m_emptyVal;

    if (m_links.empty()) {
        return;
    }

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

template <class T>
void InPin<T>::removeLink(Link* link)
{
    m_links.erase(
        std::find_if(
            m_links.begin(),
            m_links.end(),
            [link](const auto* internal_link) -> bool {
                return internal_link == link;
            }));
}

// -----------------------------------------------------------------------------------------------------------------
// OUT PIN

template <class T>
OutPin<T>::~OutPin() = default;

template <class T>
void OutPin<T>::setBehaviour(std::function<T(Node&)> new_behaviour)
{
    this->m_behaviour = std::move(new_behaviour);
}

template <class T>
void OutPin<T>::calculate(ImFlow::Node& node)
{
    m_valNext = m_behaviour(node);
}

template <class T>
void OutPin<T>::prime()
{
    m_val = m_valNext;
}

template <class T>
void OutPin<T>::setNextValue(T next_value)
{
    m_valNext = next_value;
}

template <class T>
const T&
OutPin<T>::value()
{
    return m_val;
}

template <class T>
void OutPin<T>::removeLink(Link* link)
{
    m_links.erase(
        std::find_if(
            m_links.begin(),
            m_links.end(),
            [link](const auto* internal_link) -> bool {
                return internal_link == link;
            }));
}

} // namespace ImFlow
