#ifndef IMNODEFLOW_EDITOR_CONTEXT_H
#define IMNODEFLOW_EDITOR_CONTEXT_H
#pragma once

#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/Style.h"
#include "ImNodeFlow/callback_types.h"

#include "./context_wrapper.h"
#include "./node.h"

#include <string>
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

} // namespace ImFlow::detail

#endif // IMNODEFLOW_EDITOR_CONTEXT_H
