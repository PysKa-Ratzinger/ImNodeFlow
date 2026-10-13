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
Pin& BaseNode::addIN(const std::string& name, T defaultValue, PinStyle style)
{
    return addIN_uid(PinUID::generate(), name, defaultValue, style);
}

template <typename T>
Pin& BaseNode::addIN_uid(
    const PinUID& uid,
    const std::string& name,
    T defaultValue,
    PinStyle style)
{
    m_ins.emplace_back(
        std::make_unique<InPin<T>>(
            m_flow, uid, name, defaultValue, style, this));
    return *m_ins.back();
}

template <typename T>
const T&
BaseNode::showIN(const std::string& name, T defaultValue, PinStyle style)
{
    for (auto& dyn_pin : m_dynamicIns) {
        if (dyn_pin.pin().getName() == name) {
            dyn_pin.refresh();
            return dyn_pin.pin().toIn<T>().val();
        }
    }

    m_dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(
            m_flow, PinUID::generate(), name, defaultValue, style, this));
    return m_dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
const T&
BaseNode::showIN_uid(
    const PinUID& uid,
    const std::string& name,
    T defaultValue,
    PinStyle style)
{
    for (auto& dyn_pin : m_dynamicIns) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            return dyn_pin.pin().toIn<T>().val();
        }
    }

    m_dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(
            m_flow, uid, name, defaultValue, style, this));
    return m_dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
Pin& BaseNode::addOUT(
    const std::string& name,
    std::function<T()> behaviour,
    PinStyle style)
{
    return addOUT_uid<T>(
        PinUID::generate(), name, std::move(behaviour), style);
}

template <typename T>
Pin& BaseNode::addOUT_uid(
    const PinUID& uid,
    const std::string& name,
    std::function<T()> behaviour,
    PinStyle style)
{
    m_outs.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow, uid, name, std::move(behaviour), style, this));
    return *m_outs.back();
}

template <typename T>
void BaseNode::showOUT(
    const std::string& name,
    std::function<T()> behaviour,
    PinStyle style)
{
    for (auto& dyn_pin : m_dynamicOuts) {
        if (dyn_pin.pin().getName() == name) {
            dyn_pin.refresh();
            m_dynamicOuts.back().pin().toOut<T>().setBehaviour(
                std::move(behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow,
            PinUID::generate(),
            name,
            std::move(behaviour),
            style,
            this));

    m_dynamicOuts.back().pin().toOut<T>().setBehaviour(std::move(behaviour));
}

template <typename T>
void BaseNode::showOUT_uid(
    const PinUID& uid,
    const std::string& name,
    std::function<T()> behaviour,
    PinStyle style)
{
    for (auto& dyn_pin : m_dynamicOuts) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            m_dynamicOuts.back().pin().toOut<T>().setBehaviour(
                std::move(behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_flow, uid, name, std::move(behaviour), style, this));
    m_dynamicOuts.back().pin().toOut<T>().setBehaviour(std::move(behaviour));
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
    assert(m_type == PinType_Input && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<InPin<T>&>(*this);
}

template <class T>
InPin<T> const&
Pin::toIn() const
{
    // check if casting is safe without RTTI
    assert(m_type == PinType_Input && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<InPin<T> const&>(*this);
}

template <class T>
OutPin<T>&
Pin::toOut()
{
    // check if casting is safe without RTTI
    assert(m_type == PinType_Output && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
    return static_cast<OutPin<T>&>(*this);
}

template <class T>
OutPin<T> const&
Pin::toOut() const
{
    // check if casting is safe without RTTI
    assert(m_type == PinType_Output && getTypeID() == detail::type_id<T> && "Tried to cast a pin into a different type than it was created");
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
        m_val = m_links.front()->outPin()->template toOut<T>().val();
        return;

    case PinConnStrat_AllowMultiplePreferNewest:
        m_val = m_links.back()->outPin()->template toOut<T>().val();
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
bool InPin<T>::canCreateLink(Pin& other)
{
    if (&other == this || other.getType() == PinType_Input) {
        return false;
    }

    if (m_parent == other.getParent() && !m_allowSelfConnection) {
        return false;
    }

    for (const auto* link : m_links) {
        if (link->outPin() == &other) {
            return false;
        }
    }

    // Only pins with the same type can be linked. Casts between
    // compatible types like (float and double), or (short, int, and long)
    // should be handled explicitly by user types.
    //
    // On a technical note, supporting these casts is not necessarily hard,
    // but introduces a layer of complexity which either requires RTTI (so
    // we know how to type-cast), or manual RTTI (using a compile-time
    // type_id<T>, which is possible, but we are now responsible for
    // maintaining a list of which type conversions should be possible and
    // which aren't), or it might introduce subtle programming errors
    // given the nature that different types might naturally have different
    // sizes in memory, and getting a cast wrong may result in writing
    // into out-of-bounds memory.
    //
    // Supporting connections only between same types avoids all these
    // issues, and it's not something that's prohibitibly hard to work
    // around on the client side.
    return other.getTypeID() == this->getTypeID();
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
bool OutPin<T>::canCreateLink(ImFlow::Pin& other)
{
    if (&other == this || other.getType() == PinType_Output) {
        return false;
    }

    return other.canCreateLink(*this);
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
