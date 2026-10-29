#ifndef IMNODEFLOW_PIN_BASE_H
#define IMNODEFLOW_PIN_BASE_H
#pragma once

#include "ImNodeFlow/PinConnStrat.h"
#include "ImNodeFlow/PinInfo.h"
#include "ImNodeFlow/PinStyle.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/PinUID.h"
#include "ImNodeFlow/callback_types.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>

namespace ImFlow::detail {

struct pin_base_t {
    pin_base_t(
        detail::node_t& parent,
        PinType pin_type,
        BasePinInfo info,
        std::unique_ptr<IPin> impl) noexcept;

    PinUID uid;
    std::string name;
    ImVec2 pos = ImVec2(0.f, 0.f);
    ImVec2 size = ImVec2(0.f, 0.f);
    ImVec2 pinPoint { 0.f, 0.f };
    PinType type;
    detail::node_t* parent = nullptr;
    PinStyle style;
    PinConnStrat strat;
    bool allowSelfConnection = false;
    bool socketHitboxEnabled = true;
    bool isHovered = false;
    pin_rendering_handler_t renderer;
    std::vector<link_t*> links { };
    std::unique_ptr<IPin> underlying_pin;
};

} // namespace ImFlow::detail

#endif // IMNODEFLOW_PIN_BASE_H
