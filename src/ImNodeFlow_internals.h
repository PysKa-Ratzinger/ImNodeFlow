#ifndef IM_NODE_FLOW_INTERNALS
#define IM_NODE_FLOW_INTERNALS
#pragma once

#include "ImNodeFlow.h"

namespace ImFlow {

struct ImNodeFlow::impl_t {
    std::string name;
    ContainedContext context;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } singleUseClicks;

    std::unordered_map<NodeUID, std::unique_ptr<Node>, NodeUID::Hash>
        nodes;
    std::vector<std::unique_ptr<Link>> links;

    std::function<void(Pin* dragged)> droppedLinkPopUp;
    ImGuiKey droppedLinkPupUpComboKey = ImGuiKey_None;
    Pin* droppedLinkLeft = nullptr;
    right_click_popup_handler_t rightClickPopUp;
    std::function<void(const ImVec2&)> dragdropTarget;
    Node* hoveredNodeAux = nullptr;

    Node* hoveredNode = nullptr;
    bool draggingNode = false, draggingNodeNext = false;
    Pin* hovering = nullptr;
    Pin* dragOut = nullptr;

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

} // namespace ImFlow

#endif // IM_NODE_FLOW_INTERNALS
