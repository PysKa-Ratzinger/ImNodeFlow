#ifndef IMNODEFLOW_PINTYPE_H
#define IMNODEFLOW_PINTYPE_H
#pragma once

#include <cstdint>

namespace ImFlow {

/**
 * @brief Pins type identifier
 */
enum class PinType : uint8_t {
    Input,
    Output,
};

} // namespace ImFlow

#endif // IMNODEFLOW_PINTYPE_H
