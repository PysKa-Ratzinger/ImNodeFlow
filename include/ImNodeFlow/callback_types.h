#ifndef IMNODEFLOW_CALLBACK_TYPES_H
#define IMNODEFLOW_CALLBACK_TYPES_H
#pragma once

#include "ImNodeFlow/fwd.h"

#include <functional>
#include <optional>

namespace ImFlow {

using right_click_popup_handler_t = std::function<void(std::optional<NodeRef> node)>;

} // namespace ImFlow

#endif // IMNODEFLOW_CALLBACK_TYPES_H
