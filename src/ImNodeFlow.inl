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
// BASE NODE

template <typename T>
Pin& BaseNode::addIN(InPinInfo<T> pin_info)
{
    // TODO: Make user UID is unique (or disallow the use of user provided UIDs)
    return addIN(PinUID::generate(), std::move(pin_info));
}

template <typename T>
Pin& BaseNode::addIN(const PinUID& uid, InPinInfo<T> pin_info)
{
    m_ins.emplace_back(
        std::make_unique<InPin<T>>(
            m_flow, *this, uid, std::move(pin_info)));
    return *m_ins.back();
}

template <typename T>
const T&
BaseNode::showIN(InPinInfo<T> pin_info)
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
BaseNode::showIN(const PinUID& uid, InPinInfo<T> pin_info)
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
Pin& BaseNode::addOUT(OutPinInfo<T> pin_info)
{
    // TODO: Make user UID is unique (or disallow the use of user provided UIDs)
    return addOUT<T>(PinUID::generate(), std::move(pin_info));
}

template <typename T>
Pin& BaseNode::addOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    m_outs.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow, *this, uid, std::move(pin_info)));
    return *m_outs.back();
}

template <typename T>
void BaseNode::showOUT(OutPinInfo<T> pin_info)
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
void BaseNode::showOUT(const PinUID& uid, OutPinInfo<T> pin_info)
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
const T&
BaseNode::getInVal(const PinUID& uid)
{
    return this->inPin(uid).toIn<T>().val();
}

template <typename T>
const T&
BaseNode::getInVal(const char* name)
{
    return this->inPin(name).toIn<T>().val();
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
void InPin<T>::calculate()
{
    if (m_links.empty()) {
        m_val = m_emptyVal;
        return;
    }

    switch (m_strat) {
    case PinConnStrat_AllowOnlySingleConnection:
    case PinConnStrat_AllowMultiplePreferOldest:
        m_val = m_links.front()->outPin().template toOut<T>().val();
        return;

    case PinConnStrat_AllowMultiplePreferNewest:
        m_val = m_links.back()->outPin().template toOut<T>().val();
        return;

    default:
        return;
    }
}

template <class T>
void InPin<T>::prime()
{
    m_val = m_valNext;
}

template <class T>
const T&
InPin<T>::val()
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
void OutPin<T>::setBehaviour(std::function<T()> new_behaviour)
{
    this->m_behaviour = std::move(new_behaviour);
}

template <class T>
void OutPin<T>::calculate()
{
    m_valNext = m_behaviour();
}

template <class T>
void OutPin<T>::prime()
{
    m_val = m_valNext;
}

template <class T>
const T&
OutPin<T>::val()
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
