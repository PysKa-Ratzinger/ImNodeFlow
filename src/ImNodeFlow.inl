#pragma once

#include "ImNodeFlow.h"

#include "ImNodeFlow/Context.h"
#include "ImNodeFlow/PinConnStrat.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/detail/type_id.h"

// FIXME: Remove this header
// These public API inline utilities are included by the public API file,
// and including the internal data structures introduces compile-time
// overhead for the users of the library
#include "./ImNodeFlow_internals.h"
#include "./Link.h"
#include "./uid_helpers.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace ImFlow::detail {

// -----------------------------------------------------------------------------------------------------------------
// IPIN TYPE CONVERSIONS

// -----------------------------------------------------------------------------------------------------------------
// PIN UTILS

template <typename T>
in_pin_t<T>::in_pin_t(
    ImNodeFlow& ctx,
    detail::node_t& parent,
    InPinInfo<T> info)
    : base {
        .ctx = ctx,
        .uid = detail::generate_pin_uid(),
        .name = std::move(info.name),
        .type = PinType::Input,
        .parent = &parent,
        .style = std::move(info.style),
        .strat = info.strat,
        .allowSelfConnection = info.allow_same_node_connections,
        .renderer = std::move(info.renderer),
    }
    , emptyVal { info.default_value }
    , val { info.default_value }
{
    if (info.accumulate_op) {
        this->accumulatorOp = std::move(info.accumulate_op);
    } else {
        switch (info.strat) {
        case PinConnStrat::AllowOnlySingleConnection:
        case PinConnStrat::AllowMultiplePreferOldest:
            this->accumulatorOp =
                [](bool first_pin,
                    T init,
                    T const& incoming_value) -> T {
                return incoming_value;
            };
            break;

        case PinConnStrat::AllowMultiplePreferNewest:
            this->accumulatorOp =
                [](bool first_pin,
                    T init,
                    T const& incoming_value) -> T {
                return first_pin ? incoming_value : init;
            };
        }
    }
}

template <typename T>
out_pin_t<T>::out_pin_t(
    ImNodeFlow& ctx,
    detail::node_t& parent,
    OutPinInfo<T> info)
    : base {
        .ctx = ctx,
        .uid = detail::generate_pin_uid(),
        .name = std::move(info.name),
        .type = PinType::Output,
        .parent = &parent,
        .style = std::move(info.style),
        .strat = info.strat,
        .allowSelfConnection = info.allow_same_node_connections,
        .renderer = std::move(info.renderer),
    }
    , behaviour { std::move(info.behaviour) }
    , val { }
{
}

template <typename T>
pin_base_t& in_pin_t<T>::toBase()
{
    return this->base;
}

template <typename T>
pin_base_t const& in_pin_t<T>::toBase() const
{
    return this->base;
}

template <typename T>
uint64_t in_pin_t<T>::getTypeID() const
{
    return get_type_id<T>();
}

template <typename T>
void in_pin_t<T>::calculate()
{
    T next_value = this->emptyVal;

    next_value = this->emptyVal;

    bool first = true;
    for (auto& link : this->base.links) {
        auto& out_pin = link.link()->outPin().toOut<T>();
        next_value = this->accumulatorOp(
            first,
            next_value,
            out_pin.val);
        first = false;
    }

    this->valNext = next_value;
}

template <typename T>
void in_pin_t<T>::prime()
{
    this->val = this->valNext;
}

template <typename T>
pin_base_t& out_pin_t<T>::toBase()
{
    return this->base;
}

template <typename T>
pin_base_t const& out_pin_t<T>::toBase() const
{
    return this->base;
}

template <typename T>
uint64_t out_pin_t<T>::getTypeID() const
{
    return get_type_id<T>();
}

template <typename T>
void out_pin_t<T>::calculate()
{
    this->valNext = this->behaviour(NodeRef { *this->base.parent });
}

template <typename T>
void out_pin_t<T>::prime()
{
    this->val = this->valNext;
}

} // namespace ImFlow::detail

namespace ImFlow {

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

template <bool TIsConst>
template <typename T>
InPinRef<T, TIsConst>
NodeRefImpl<TIsConst>::addIN(InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    m_node.get().ins.emplace_back(std::make_unique<detail::in_pin_t<T>>(
        m_node.get().ctx,
        *this,
        std::move(pin_info)));
    return PinRef<TIsConst> { *m_node.get().ins.back() }.template toIn<T>();
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
        std::make_unique<detail::in_pin_t<T>>(
            m_node.get().ctx, *this, std::move(pin_info)));
    return m_node.get().dynamicIns.back().pin().template toIn<T>().val();
}

template <bool TIsConst>
template <typename T>
OutPinRef<T, TIsConst>
NodeRefImpl<TIsConst>::addOUT(OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    m_node.get().outs.emplace_back(
        std::make_unique<detail::out_pin_t<T>>(
            m_node.get().ctx, *this, std::move(pin_info)));
    return OutPinRef<T, TIsConst> {
        m_node.get().outs.back()->template toOut<T>()
    };
}

template <bool TIsConst>
template <typename T>
void NodeRefImpl<TIsConst>::showOUT(OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicOuts) {
        if (dyn_pin.pin().toBase().name == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            dyn_pin.pin().template toOut<T>().behaviour
                = std::move(pin_info.behaviour);
            return;
        }
    }

    m_node.get().dynamicOuts.emplace_back(
        std::make_unique<detail::out_pin_t<T>>(
            m_node.get().ctx, *this, std::move(pin_info)));
}

template <bool TIsConst>
template <typename T>
std::optional<InPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::inPin(const PinUID& uid)
{
    auto pin = this->inPin(uid);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    detail::IPin& ipin_ref = *pin;
    return InPinRef<T, TIsConst> { ipin_ref.toIn<T>() };
}

template <bool TIsConst>
template <typename T>
std::optional<InPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::inPin(const char* name)
{
    auto pin = this->inPin(name);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    detail::IPin& ipin_ref = *pin;
    return InPinRef<T, TIsConst> { ipin_ref.toIn<T>() };
}

template <bool TIsConst>
template <typename T>
std::optional<OutPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::outPin(const PinUID& uid)
{
    auto pin = this->outPin(uid);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    detail::IPin& ipin_ref = *pin;
    return OutPinRef<T, TIsConst> { ipin_ref.toOut<T>() };
}

template <bool TIsConst>
template <typename T>
std::optional<OutPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::outPin(const char* name)
{
    auto pin = this->outPin(name);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    detail::IPin& ipin_ref = *pin;
    return OutPinRef<T, TIsConst> { ipin_ref.toOut<T>() };
}

} // namespace ImFlow
