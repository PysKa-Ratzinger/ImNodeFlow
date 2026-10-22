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
NodeRef ImNodeFlow::emplaceNode(const ImVec2& grid_coords, Params&&... args)
{
    return addNode(
        grid_coords,
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

template <typename T, typename... Params>
NodeRef ImNodeFlow::emplaceNodeAt(const ImVec2& screen_coords, Params&&... args)
{
    return placeNodeAt(
        screen_coords,
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

template <typename T, typename... Params>
NodeRef ImNodeFlow::emplaceNodeOnMouse(Params&&... args)
{
    return placeNode(
        NodeInfo {
            .behaviour = std::make_unique<T>(std::forward<Params>(args)...),
        });
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

template <bool TIsConst>
template <typename T>
InPin<T>&
NodeRefImpl<TIsConst>::addIN(InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    return addIN(PinUID::generate(), std::move(pin_info));
}

template <bool TIsConst>
template <typename T>
InPin<T>&
NodeRefImpl<TIsConst>::addIN(const PinUID& uid, InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    m_node.get().ins.emplace_back(
        std::make_unique<InPin<T>>(
            m_node.get().ctx, uid, *this, std::move(pin_info)));
    return m_node.get().ins.back()->template toIn<T>();
}

template <bool TIsConst>
template <typename T>
const T&
NodeRefImpl<TIsConst>::showIN(InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicIns) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return dyn_pin.pin().template toIn<T>().val();
        }
    }

    m_node.get().dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(
            m_node.get().ctx, PinUID::generate(), *this, std::move(pin_info)));
    return m_node.get().dynamicIns.back().pin().template toIn<T>().val();
}

template <bool TIsConst>
template <typename T>
const T&
NodeRefImpl<TIsConst>::showIN(const PinUID& uid, InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicIns) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return dyn_pin.pin().template toIn<T>().val();
        }
    }

    m_node.get().dynamicIns.emplace_back(
        std::make_unique<InPin<T>>(
            m_node.get().ctx, uid, *this, std::move(pin_info)));
    return m_node.get().dynamicIns.back().pin().template toIn<T>().val();
}

template <bool TIsConst>
template <typename T>
OutPin<T>&
NodeRefImpl<TIsConst>::addOUT(OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    return addOUT<T>(PinUID::generate(), std::move(pin_info));
}

template <bool TIsConst>
template <typename T>
OutPin<T>&
NodeRefImpl<TIsConst>::addOUT(const PinUID& uid, OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    m_node.get().outs.emplace_back(
        std::make_unique<OutPin<T>>(
            m_node.get().ctx, uid, *this, std::move(pin_info)));
    return m_node.get().outs.back()->template toOut<T>();
}

template <bool TIsConst>
template <typename T>
void NodeRefImpl<TIsConst>::showOUT(OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicOuts) {
        if (dyn_pin.pin().getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().template toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_node.get().dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_node.get().ctx, PinUID::generate(), *this, std::move(pin_info)));
}

template <bool TIsConst>
template <typename T>
void NodeRefImpl<TIsConst>::showOUT(const PinUID& uid, OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicOuts) {
        if (dyn_pin.pin().getUid() == uid) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().template toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_node.get().dynamicOuts.emplace_back(
        std::make_unique<OutPin<T>>(
            m_node.get().ctx, uid, *this, std::move(pin_info)));
    m_node.get().dynamicOuts.back().pin().template toOut<T>().behaviour(
        std::move(pin_info.behaviour));
}

template <bool TIsConst>
template <typename T>
auto NodeRefImpl<TIsConst>::inPin(const PinUID& uid) -> in_pin_pointer_t<T>
{
    auto* pin = this->inPin(uid);
    return pin ? &pin->template toIn<T>() : nullptr;
}

template <bool TIsConst>
template <typename T>
auto NodeRefImpl<TIsConst>::inPin(const char* name) -> in_pin_pointer_t<T>
{
    auto* pin = this->inPin(name);
    return pin ? &pin->template toIn<T>() : nullptr;
}

template <bool TIsConst>
template <typename T>
auto NodeRefImpl<TIsConst>::outPin(const PinUID& uid) -> out_pin_pointer_t<T>
{
    auto* pin = this->outPin(uid);
    return pin ? &pin->template toOut<T>() : nullptr;
}

template <bool TIsConst>
template <typename T>
auto NodeRefImpl<TIsConst>::outPin(const char* name) -> out_pin_pointer_t<T>
{
    auto* pin = this->outPin(name);
    return pin ? &pin->template toOut<T>() : nullptr;
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
InPin<T>::value() const
{
    return m_val;
}

// -----------------------------------------------------------------------------------------------------------------
// OUT PIN

template <class T>
void OutPin<T>::calculate()
{
    m_valNext = m_behaviour(getParent());
}

template <class T>
void OutPin<T>::prime()
{
    m_val = m_valNext;
}

template <class T>
const T&
OutPin<T>::value() const
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
