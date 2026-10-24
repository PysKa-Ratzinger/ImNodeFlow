#ifndef IMNODEFLOW_PINUID_H
#define IMNODEFLOW_PINUID_H
#pragma once

#include <cstdint>

namespace ImFlow {

struct PinUID {
    uint64_t id;

    [[nodiscard]] constexpr bool operator==(PinUID const& other) const noexcept
    {
        return this->id == other.id;
    }
};

} // namespace ImFlow

#endif // IMNODEFLOW_PINUID_H
