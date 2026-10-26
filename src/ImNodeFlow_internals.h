#ifndef IM_NODE_FLOW_INTERNALS
#define IM_NODE_FLOW_INTERNALS
#pragma once

#include "ImNodeFlow.h"

#include "ImNodeFlow/PinUID.h"
#include "ImNodeFlow/Style.h"
#include "ImNodeFlow/callback_types.h"
#include "ImNodeFlow/detail/IPin.h"

#include "./node.h"
#include "./pin_base.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace ImFlow::detail {

template <typename T>
struct in_pin_t final : public IPin {
    pin_base_t base;
    T emptyVal;
    T val;
    T valNext;
    std::function<T(bool first_pin, T init, T const& incoming_value)>
        accumulatorOp;

    in_pin_t(
        ImNodeFlow& ctx,
        detail::node_t& parent,
        InPinInfo<T> pin_info);

    pin_base_t& toBase() final;
    [[nodiscard]] pin_base_t const& toBase() const final;
    [[nodiscard]] uint64_t getTypeID() const final;
    void calculate() final;
    void prime() final;
};

template <typename T>
struct out_pin_t final : public IPin {
    pin_base_t base;
    std::function<T(NodeRef)> behaviour;
    T val;
    T valNext;

    out_pin_t(
        ImNodeFlow& ctx,
        detail::node_t& parent,
        OutPinInfo<T> pin_info);

    pin_base_t& toBase() final;
    [[nodiscard]] pin_base_t const& toBase() const final;
    [[nodiscard]] uint64_t getTypeID() const final;
    void calculate() final;
    void prime() final;
};

} // namespace ImFlow::detail

#endif // IM_NODE_FLOW_INTERNALS
