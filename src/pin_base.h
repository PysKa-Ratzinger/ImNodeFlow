#ifndef IMNODEFLOW_PIN_BASE_H
#define IMNODEFLOW_PIN_BASE_H
#pragma once

#include "ImNodeFlow/PinConnStrat.h"
#include "ImNodeFlow/PinStyle.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/PinUID.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "./PinConnectionHolder.h"

#include "imgui.h"

#include <functional>
#include <string>
#include <vector>

namespace ImFlow::detail {

struct pin_base_t {
    std::reference_wrapper<ImNodeFlow> ctx;
    PinUID uid;
    std::string name;
    ImVec2 pos = ImVec2(0.f, 0.f);
    ImVec2 size = ImVec2(0.f, 0.f);
    PinType type;
    detail::node_t* parent = nullptr;
    PinStyle style;
    PinConnStrat strat;
    bool allowSelfConnection = false;
    bool socketHitboxEnabled = true;
    std::function<void(PinRef<> p)> renderer;
    std::vector<PinConnectionHolder> links { };
};

} // namespace ImFlow::detail

#endif // IMNODEFLOW_PIN_BASE_H
