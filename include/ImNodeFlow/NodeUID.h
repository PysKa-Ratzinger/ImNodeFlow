#ifndef IMNODEFLOW_NODEUID_H
#define IMNODEFLOW_NODEUID_H
#pragma once

namespace ImFlow {

struct NodeUID {
    uint64_t id;

    [[nodiscard]] constexpr bool operator==(NodeUID const& other) const noexcept
    {
        return this->id == other.id;
    }
};

} // namespace ImFlow

#endif // IMNODEFLOW_NODEUID_H
