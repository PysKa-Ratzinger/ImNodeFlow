#ifndef IMNODEFLOW_PININFO_H
#define IMNODEFLOW_PININFO_H
#pragma once

#include "ImNodeFlow/PinConnStrat.h"
#include "ImNodeFlow/PinStyle.h"
#include "ImNodeFlow/callback_types.h"
#include "ImNodeFlow/fwd.h"

#include <functional>
#include <string>

namespace ImFlow {

struct BasePinInfo {
    /// Name of the pin
    std::string name;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections = false;
    /// Style to apply to the pin
    PinStyle style = PinStyles::default_style;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat::AllowOnlySingleConnection;
    /// Custom render function
    pin_rendering_handler_t renderer { };
};

/**
 * @brief Description of parameters customizable during pin creation
 */
template <typename T>
struct InPinInfo {
    /// Name of the pin
    std::string name;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections = false;
    /// Style to apply to the pin
    PinStyle style = PinStyles::default_style;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat::AllowOnlySingleConnection;
    /// Custom render function
    pin_rendering_handler_t renderer { };
    /// Default value the input pin will return when not connected
    /// to any output pin
    T default_value { };
    /// When an input pin supports multiple inputs, the pin's value will
    /// be calculated by first initializing it to the default value, and
    /// then calling this accumulator function for every connected pin
    /// (from oldest to newest).
    in_pin_accumulator_t<T> accumulate_op { };
};

/**
 * @brief Description of parameters customizable during pin creation
 */
template <typename T>
struct OutPinInfo {
    /// Name of the pin
    std::string name;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections = true;
    /// Style to apply to the pin
    PinStyle style = PinStyles::default_style;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat::AllowMultipleConnections;
    /// Custom render function
    pin_rendering_handler_t renderer { };
    /// Default value the output pin will return before the behaviour has a
    /// chance to change it.
    T default_value { };
    /// Behaviour of the pin when a value update is requested
    out_pin_behaviour_t<T> behaviour;
};

} // namespace ImFlow

#endif // IMNODEFLOW_PININFO_H
