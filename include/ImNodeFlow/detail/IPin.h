#ifndef IMNODEFLOW_DETAIL_IPIN_H
#define IMNODEFLOW_DETAIL_IPIN_H
#pragma once

#include "ImNodeFlow/detail/fwd.h"

#include <cstdint>

namespace ImFlow::detail {

class IPin {
public:
    virtual ~IPin() = default;

    [[nodiscard]] virtual uint64_t getTypeID() const = 0;
    virtual pin_base_t& toBase() = 0;
    [[nodiscard]] virtual pin_base_t const& toBase() const = 0;
    virtual void calculate() = 0;
    virtual void prime() = 0;

    template <typename T>
    in_pin_t<T>& toIn();
    template <typename T>
    in_pin_t<T> const& toIn() const;

    template <typename T>
    out_pin_t<T>& toOut();
    template <typename T>
    out_pin_t<T> const& toOut() const;
};

} // namespace ImFlow::detail

#include "./IPin.inl"

#endif // IMNODEFLOW_DETAIL_IPIN_H
