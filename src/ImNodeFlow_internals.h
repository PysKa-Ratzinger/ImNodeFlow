#ifndef IM_NODE_FLOW_INTERNALS
#define IM_NODE_FLOW_INTERNALS
#pragma once

#include "ImNodeFlow.h"

#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/PinUID.h"
#include "ImNodeFlow/Style.h"
#include "ImNodeFlow/callback_types.h"
#include "ImNodeFlow/detail/IPin.h"

#include "./PinConnectionHolder.h"
#include "./context_wrapper.h"
#include "./node.h"
#include "./pin_base.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

template <>
struct std::hash<ImFlow::NodeUID> {
    constexpr std::size_t operator()(const ImFlow::NodeUID& n) const noexcept
    {
        return std::hash<uint64_t> { }(n.id);
    }
};

namespace ImFlow::detail {

struct editor_context_t {
    std::string name;
    ContainedContext context;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } singleUseClicks;

    std::unordered_map<NodeUID, node_t> nodes;
    std::vector<std::unique_ptr<Link>> links;

    std::function<void(PinRef<> dragged)> droppedLinkPopUp;
    ImGuiKey droppedLinkPupUpComboKey = ImGuiKey_None;
    IPin* droppedLinkLeft = nullptr;
    right_click_popup_handler_t rightClickPopUp;
    std::function<void(const ImVec2&)> dragdropTarget;
    node_t* hoveredNodeAux = nullptr;

    node_t* hoveredNode = nullptr;
    bool draggingNode = false, draggingNodeNext = false;
    IPin* hovering = nullptr;
    IPin* dragOut = nullptr;

    InfStyler style;

    struct SelectionBox {
        bool active = false;
        ImVec2 selectionStart;
        ImU32 selectionColor = IM_COL32(100, 150, 255, 50);
        ImU32 borderColor = IM_COL32(100, 150, 255, 200);
    };

    SelectionBox box;

    bool draggingCanvas = false;
};

template <typename T>
struct in_pin_t final : public IPin {
    pin_base_t base;
    T emptyVal;
    T val;
    T valNext;
    std::function<T(bool first_pin, T init, T const& incoming_value)>
        accumulatorOp;

    in_pin_t(
        ImNodeFlow& ctx,
        detail::node_t& parent,
        InPinInfo<T> pin_info);

    pin_base_t& toBase() final;
    [[nodiscard]] pin_base_t const& toBase() const final;
    [[nodiscard]] uint64_t getTypeID() const final;
    void calculate() final;
    void prime() final;
};

template <typename T>
struct out_pin_t final : public IPin {
    pin_base_t base;
    std::function<T(NodeRef)> behaviour;
    T val;
    T valNext;

    out_pin_t(
        ImNodeFlow& ctx,
        detail::node_t& parent,
        OutPinInfo<T> pin_info);

    pin_base_t& toBase() final;
    [[nodiscard]] pin_base_t const& toBase() const final;
    [[nodiscard]] uint64_t getTypeID() const final;
    void calculate() final;
    void prime() final;
};

} // namespace ImFlow::detail

#endif // IM_NODE_FLOW_INTERNALS
