#pragma once

#include "ImNodeFlow.h"

#include <cassert>
#include <cstdint>
#include <memory>

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
InPin<T>&
BaseNode::addIN(
    const std::string& name,
    T defReturn,
    std::shared_ptr<IFilter> filter,
    PinStyle style)
{
    return addIN_uid(
        PinUID::generate(), name, defReturn, std::move(filter), style);
}

template <typename T>
InPin<T>&
BaseNode::addIN_uid(
    const PinUID& uid,
    const std::string& name,
    T defReturn,
    std::shared_ptr<IFilter> filter,
    PinStyle style)
{
    auto p = std::make_shared<InPin<T>>(
        m_flow, uid, name, defReturn, std::move(filter), style, this);
    m_ins.emplace_back(p);
    return *p;
}

template <typename T>
const T&
BaseNode::showIN(
    const std::string& name,
    T defReturn,
    std::shared_ptr<IFilter> filter,
    PinStyle style)
{
    for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicIns) {
        if (p.second->getName() == name) {
            p.first = 1;
            return static_cast<InPin<T>*>(p.second.get())->val();
        }
    }

    m_dynamicIns.emplace_back(
        std::make_pair(
            1,
            std::make_shared<InPin<T>>(
                m_flow,
                PinUID::generate(),
                name,
                defReturn,
                std::move(filter),
                style,
                this)));
    return static_cast<InPin<T>*>(m_dynamicIns.back().second.get())->val();
}

template <typename T>
const T&
BaseNode::showIN_uid(
    const PinUID& uid,
    const std::string& name,
    T defReturn,
    std::shared_ptr<IFilter> filter,
    PinStyle style)
{
    for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicIns) {
        if (p.second->getUid() == uid) {
            p.first = 1;
            return static_cast<InPin<T>*>(p.second.get())->val();
        }
    }

    m_dynamicIns.emplace_back(
        std::make_pair(
            1,
            std::make_shared<InPin<T>>(
                m_flow,
                uid,
                name,
                defReturn,
                std::move(filter),
                style,
                this)));
    return static_cast<InPin<T>*>(m_dynamicIns.back().second.get())->val();
}

template <typename T>
OutPin<T>&
BaseNode::addOUT(const std::string& name, PinStyle style)
{
    return addOUT_uid<T>(PinUID::generate(), name, style);
}

template <typename T>
OutPin<T>&
BaseNode::addOUT_uid(const PinUID& uid, const std::string& name, PinStyle style)
{
    auto p = std::make_shared<OutPin<T>>(m_flow, uid, name, style, this);
    m_outs.emplace_back(p);
    return *p;
}

template <typename T>
void BaseNode::showOUT(
    const std::string& name,
    std::function<T()> behaviour,
    PinStyle style)
{
    for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicOuts) {
        if (p.second->getName() == name) {
            p.first = 2;
            static_cast<OutPin<T>*>(
                m_dynamicOuts.back().second.get())
                ->behaviour(std::move(behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_pair(
            2,
            std::make_shared<OutPin<T>>(
                m_flow, PinUID::generate(), name, style, this)));
    static_cast<OutPin<T>*>(m_dynamicOuts.back().second.get())
        ->behaviour(std::move(behaviour));
}

template <typename T>
void BaseNode::showOUT_uid(
    const PinUID& uid,
    const std::string& name,
    std::function<T()> behaviour,
    PinStyle style)
{
    for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicOuts) {
        if (p.second->getUid() == uid) {
            p.first = 2;
            static_cast<OutPin<T>*>(
                m_dynamicOuts.back().second.get())
                ->behaviour(std::move(behaviour));
            return;
        }
    }

    m_dynamicOuts.emplace_back(
        std::make_pair(
            2,
            std::make_shared<OutPin<T>>(
                m_flow, uid, name, style, this)));
    static_cast<OutPin<T>*>(m_dynamicOuts.back().second.get())
        ->behaviour(std::move(behaviour));
}

template <typename T>
const T&
BaseNode::getInVal(const PinUID& uid)
{
    return static_cast<InPin<T>&>(this->inPin(uid)).val();
}

template <typename T>
const T&
BaseNode::getInVal(const char* name)
{
    return static_cast<InPin<T>&>(this->inPin(name)).val();
}

// -----------------------------------------------------------------------------------------------------------------
// IN PIN

template <class T>
void InPin<T>::calculate()
{
    if (m_link.expired()) {
        m_valNext = m_emptyVal;
    } else {
        m_valNext = reinterpret_cast<OutPin<T>*>(m_link.lock()->outPin())
                        ->val();
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
InPin<T>::~InPin()
{
    if (auto l = m_link.lock()) {
        l->markForDeletion();
    }
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

    if (!m_link.expired() && m_link.lock()->outPin() == &other) {
        m_link.reset();
        return false;
    }

    return (*m_filter)(other, *this);
}

template <class T>
void InPin<T>::setLink(std::weak_ptr<Link> link)
{
    // Only a single link can be connected at this time
    this->deleteLink();
    m_link = std::move(link);
}

// -----------------------------------------------------------------------------------------------------------------
// OUT PIN

template <class T>
OutPin<T>::~OutPin()
{
    std::vector<std::weak_ptr<Link>> links = std::move(m_links);
    for (auto& link : links) {
        if (auto l = link.lock()) {
            l->markForDeletion();
        }
    }
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
void OutPin<T>::setLink(std::weak_ptr<Link> link)
{
    m_links.emplace_back(std::move(link));
}

template <class T>
void OutPin<T>::deleteLink()
{
    m_links.erase(
        std::remove_if(
            m_links.begin(),
            m_links.end(),
            [](const std::weak_ptr<Link>& l) { return l.expired(); }),
        m_links.end());
}

} // namespace ImFlow
