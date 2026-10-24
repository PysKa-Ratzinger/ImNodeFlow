#ifndef IMNODEFLOW_SRC_UID_HELPERS_H
#define IMNODEFLOW_SRC_UID_HELPERS_H
#pragma once

#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/PinUID.h"

#include <cstdint>

namespace ImFlow::detail {

PinUID generate_pin_uid();

NodeUID generate_node_uid();

} // namespace ImFlow::detail

#endif // IMNODEFLOW_SRC_UID_HELPERS_H
