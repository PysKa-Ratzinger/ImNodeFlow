#ifndef IMNODEFLOW_PINCONNSTRAT_H
#define IMNODEFLOW_PINCONNSTRAT_H
#pragma once

#include <cstdint>

namespace ImFlow {

/**
 * @brief Strategy to use for input pins regarding multiple connections
 */
enum class PinConnStrat : uint8_t {
    /// When a new link is created, all other links connecting to this pin
    /// are deleted.
    AllowOnlySingleConnection,

    /// Multiple links are allowed.
    ///
    /// When applied to an input pin, if an accumulate operation is not
    /// provided, it will default to fetch the value from the oldest
    /// connected link.
    ///
    /// The behaviour for an output pin does not change besides allowing
    /// the creation of multiple links.
    AllowMultipleConnections,
};

} // namespace ImFlow

#endif // IMNODEFLOW_PINCONNSTRAT_H
