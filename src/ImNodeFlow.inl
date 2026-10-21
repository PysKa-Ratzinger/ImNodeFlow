#pragma once

#include "ImNodeFlow.h"

// FIXME: Remove this header
// These public API inline utilities are included by the public API file,
// and including the internal data structures introduces compile-time
// overhead for the users of the library
#include "ImNodeFlow_internals.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>

namespace ImFlow::detail {

inline uint64_t
next_type_id()
{
    static uint64_t id = 0;
    return ++id;
}

template <typename T>
inline uint64_t type_id = next_type_id();

/**
 * @brief Returns a locally unique identifier for a specified type
 *
 * This allows pins to connect only to pins of the same type
 */
template <typename T>
inline static uint64_t get_type_id()
{
    return type_id<T>;
}

} // namespace ImFlow::detail

namespace ImFlow {

// -----------------------------------------------------------------------------------------------------------------
// FLOW CONTEXT

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNode(const ImVec2& grid_coords, Params&&... args)
{
    return addNode(
        grid_coords,
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNodeAt(const ImVec2& screen_coords, Params&&... args)
{
    return placeNodeAt(
        screen_coords,
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

template <typename T, typename... Params>
Node& ImNodeFlow::emplaceNodeOnMouse(Params&&... args)
{
    return placeNode(
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

template <typename T>
InPin<T>&
Node::addIN(InPinInfo<T> pin_info)
{
    return addIN(PinUID::generate(), std::move(pin_info));
}

template <typename T>
InPin<T>&
Node::addIN(const PinUID& uid, InPinInfo<T> pin_info)
{
    m_impl->ins.emplace_back(
        std::make_unique<InPin<T>>(m_impl->ctx, uid, this, std::move(pin_info)));
    return m_impl->ins.back()->toIn<T>();
}

template <typename T>
const T&
Node::showIN(InPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_impl->dynamicIns) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return dyn_pin.pin().toIn<T>().val();
        }
    }

    m_impl->dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(
            m_impl->ctx, PinUID::generate(), this, std::move(pin_info)));
    return m_impl->dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
const T&
Node::showIN(const PinUID& uid, InPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_impl->dynamicIns) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return dyn_pin.pin().toIn<T>().val();
        }
    }

    m_impl->dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(m_impl->ctx, uid, this, std::move(pin_info)));
    return m_impl->dynamicIns.back().pin().toIn<T>().val();
}

template <typename T>
OutPin<T>&
Node::addOUT(OutPinInfo<T> pin_info)
{
    return addOUT<T>(PinUID::generate(), std::move(pin_info));
}

template <typename T>
OutPin<T>&
Node::addOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    m_impl->outs.emplace_back(
        std::make_unique<OutPin<T>>(
            m_impl->ctx, uid, this, std::move(pin_info)));
    return m_impl->outs.back()->toOut<T>();
}

template <typename T>
void Node::showOUT(OutPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_impl->dynamicOuts) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_impl->dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_impl->ctx, PinUID::generate(), this, std::move(pin_info)));
}

template <typename T>
void Node::showOUT(const PinUID& uid, OutPinInfo<T> pin_info)
{
    for (auto& dyn_pin : m_impl->dynamicOuts) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_impl->dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_impl->ctx, uid, this, std::move(pin_info)));
    m_impl->dynamicOuts.back().pin().toOut<T>().behaviour(
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
uint64_t InPin<T>::getTypeID() const
{
    return detail::get_type_id<T>();
}

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

template <class T>
uint64_t
OutPin<T>::getTypeID() const
{
    return detail::get_type_id<T>();
}

} // namespace ImFlow
