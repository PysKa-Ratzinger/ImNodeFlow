#include "ImNodeFlow.h"

#include <imgui_internal.h>

namespace ImFlow {

// -----------------------------------------------------------------------------------------------------------------
// PinUID

PinUID::PinUID(uint64_t value) noexcept
    : id(value)
{
}

PinUID
PinUID::generate() noexcept
{
    static uint64_t id = 0;

    return PinUID { id };
}

bool PinUID::operator==(PinUID const& other) const noexcept
{
    return this->id == other.id;
}

// -----------------------------------------------------------------------------------------------------------------
// PIN

inline void
Pin::drawSocket()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 tl = pinPoint() - ImVec2(m_style->socket_radius, m_style->socket_radius);
    ImVec2 br = pinPoint() + ImVec2(m_style->socket_radius, m_style->socket_radius);

    if (isConnected())
        draw_list->AddCircleFilled(
            pinPoint(),
            m_style->socket_connected_radius,
            m_style->color,
            m_style->socket_shape);
    else {
        if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
            draw_list->AddCircle(
                pinPoint(),
                m_style->socket_hovered_radius,
                m_style->color,
                m_style->socket_shape,
                m_style->socket_thickness);
        else
            draw_list->AddCircle(
                pinPoint(),
                m_style->socket_radius,
                m_style->color,
                m_style->socket_shape,
                m_style->socket_thickness);
    }

    if (ImGui::IsMouseHoveringRect(tl, br)) {
        m_is_hovered = true;
    }
}

inline void
Pin::drawDecoration()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (ImGui::IsItemHovered())
        draw_list->AddRectFilled(
            m_pos - m_style->extra.padding,
            m_pos + m_size + m_style->extra.padding,
            m_style->extra.bg_hover_color,
            m_style->extra.bg_radius);
    else
        draw_list->AddRectFilled(
            m_pos - m_style->extra.padding,
            m_pos + m_size + m_style->extra.padding,
            m_style->extra.bg_color,
            m_style->extra.bg_radius);
    draw_list->AddRect(
        m_pos - m_style->extra.padding,
        m_pos + m_size + m_style->extra.padding,
        m_style->extra.border_color,
        m_style->extra.bg_radius,
        0,
        m_style->extra.border_thickness);
}

inline void
Pin::update(ImNodeFlow& ctx)
{
    m_is_hovered = false;

    // Custom rendering
    if (m_renderer) {
        ImGui::BeginGroup();
        m_renderer(this);
        ImGui::EndGroup();
        m_size = ImGui::GetItemRectSize();
        if (ImGui::IsItemHovered()) {
            m_is_hovered = true;
        }
        return;
    }

    ImGui::SetCursorPos(m_pos);
    ImGui::Text("%s", m_name.c_str());
    m_size = ImGui::GetItemRectSize();

    drawDecoration();
    drawSocket();

    if (ImGui::IsItemHovered()) {
        m_is_hovered = true;
    }
}

void Pin::linkTo(Pin& other)
{
    if (&m_flow.get() != &other.m_flow.get()) {
        // Cannot link pins across different contexts
        return;
    }

    m_flow.get().createLink(*this, other);
}

// -----------------------------------------------------------------------------------------------------------------
// LINK

void Link::update(ImNodeFlow& ctx)
{
    ImVec2 start = m_out_pin->pinPoint();
    ImVec2 end = m_in_pin->pinPoint();
    float thickness = m_out_pin->getStyle()->extra.link_thickness;

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        m_selected = false;

    if (detail::smart_bezier_collider(
            ImGui::GetMousePos(), start, end, 2.5)) {
        m_hovered = true;
        thickness = m_out_pin->getStyle()->extra.link_hovered_thickness;
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            m_selected = true;
        }
    } else {
        m_hovered = false;
    }

    if (m_selected)
        detail::smart_bezier(
            start,
            end,
            m_out_pin->getStyle()->extra.outline_color,
            thickness + m_out_pin->getStyle()->extra.link_selected_outline_thickness);
    detail::smart_bezier(
        start, end, m_out_pin->getStyle()->color, thickness);

    if (m_selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        m_in_pin->deleteLink();
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

void BaseNode::dropIN(const PinUID& uid)
{
    m_ins.erase(
        std::find_if(m_ins.begin(), m_ins.end(), [&](auto pin) -> bool {
            return pin->getUid() == uid;
        }));
}

void BaseNode::dropIN(const char* name)
{
    m_ins.erase(
        std::remove_if(
            m_ins.begin(),
            m_ins.end(),
            [&](auto pin) -> bool { return pin->getName() == name; }),
        m_ins.end());
}

void BaseNode::dropOUT(const PinUID& uid)
{
    m_outs.erase(
        std::find_if(
            m_outs.begin(),
            m_outs.end(),
            [&uid](auto const& pin) -> bool {
                return pin->getUid() == uid;
            }));
}

inline void
BaseNode::dropOUT(const char* name)
{
    m_outs.erase(
        std::remove_if(
            m_outs.begin(),
            m_outs.end(),
            [&name](auto const& pin) -> bool {
                return pin->getName() == name;
            }),
        m_outs.end());
}

Pin& BaseNode::inPin(const PinUID& uid)
{
    auto it = std::find_if(m_ins.begin(), m_ins.end(), [&uid](const auto& p) {
        return p->getUid() == uid;
    });
    assert(it != m_ins.end() && "Pin UID not found!");
    return **it;
}

Pin& BaseNode::inPin(const char* name)
{
    auto it = std::find_if(
        m_ins.begin(), m_ins.end(), [&name](const auto& p) {
            return p->getName() == name;
        });
    assert(it != m_ins.end() && "Pin with specified name not found!");
    return **it;
}

Pin& BaseNode::outPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_outs.begin(), m_outs.end(), [&uid](const auto& p) {
            return p->getUid() == uid;
        });
    assert(it != m_outs.end() && "Pin UID not found!");
    return **it;
}

Pin& BaseNode::outPin(const char* name)
{
    auto it = std::find_if(
        m_outs.begin(), m_outs.end(), [&name](const auto& p) {
            return p->getName() == name;
        });
    assert(it != m_outs.end() && "Pin with specified name not found!");
    return **it;
}

void BaseNode::calculate()
{
    for (auto& pin : m_outs) {
        pin->calculate();
    }
    for (auto& pin : m_dynamicOuts) {
        pin.second->calculate();
    }
    for (auto& pin : m_ins) {
        pin->calculate();
    }
    for (auto& pin : m_dynamicIns) {
        pin.second->calculate();
    }
}

void BaseNode::prime()
{
    for (auto& pin : m_outs) {
        pin->prime();
    }
    for (auto& pin : m_dynamicOuts) {
        pin.second->prime();
    }
    for (auto& pin : m_ins) {
        pin->prime();
    }
    for (auto& pin : m_dynamicIns) {
        pin.second->prime();
    }
}

void BaseNode::update(ImNodeFlow& ctx)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::PushID(this);
    ImVec2 offset = ctx.grid2screen({ 0.f, 0.f });
    ImVec2 paddingTL = { m_style->padding.x, m_style->padding.y };
    ImVec2 paddingBR = { m_style->padding.z, m_style->padding.w };

    draw_list->ChannelsSetCurrent(1); // Foreground
    ImGui::SetCursorScreenPos(offset + m_pos);

    ImGui::BeginGroup();

    // Header
    ImGui::BeginGroup();
    ImGui::TextColored(m_style->header_title_color, "%s", m_title.c_str());
    ImGui::Spacing();
    ImGui::EndGroup();
    float headerH = ImGui::GetItemRectSize().y;
    float titleW = ImGui::GetItemRectSize().x;

    // Inputs
    if (!m_ins.empty() || !m_dynamicIns.empty()) {
        ImGui::BeginGroup();
        for (auto& p : m_ins) {
            p->setPos(ImGui::GetCursorPos());
            p->update(ctx);
        }
        for (auto& p : m_dynamicIns) {
            if (p.first == 1) {
                p.second->setPos(ImGui::GetCursorPos());
                p.second->update(ctx);
                p.first = 0;
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();
    }

    // Content
    ImGui::BeginGroup();
    draw();
    ImGui::Dummy(ImVec2(0.f, 0.f));
    ImGui::EndGroup();
    ImGui::SameLine();

    // Outputs
    float maxW = 0.0f;
    for (auto& p : m_outs) {
        float w = p->calcWidth();
        if (w > maxW)
            maxW = w;
    }
    for (auto& p : m_dynamicOuts) {
        float w = p.second->calcWidth();
        if (w > maxW)
            maxW = w;
    }
    ImGui::BeginGroup();
    for (auto& p : m_outs) {
        // FIXME: This looks horrible
        if ((m_pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            p->setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p->calcWidth(), 0.f));
        else
            p->setPos(ImVec2(
                (m_pos + ImVec2(titleW - p->calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        p->update(ctx);
    }
    for (auto& p : m_dynamicOuts) {
        // FIXME: This looks horrible
        if ((m_pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            p.second->setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p.second->calcWidth(), 0.f));
        else
            p.second->setPos(ImVec2(
                (m_pos + ImVec2(titleW - p.second->calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        p.second->update(ctx);
        p.first -= 1;
    }

    ImGui::EndGroup();

    ImGui::EndGroup();
    m_size = ImGui::GetItemRectSize();
    ImVec2 headerSize = ImVec2(m_size.x + paddingBR.x, headerH);

    // Background
    draw_list->ChannelsSetCurrent(0);
    draw_list->AddRectFilled(
        offset + m_pos - paddingTL,
        offset + m_pos + m_size + paddingBR,
        m_style->bg,
        m_style->radius);
    draw_list->AddRectFilled(
        offset + m_pos - paddingTL,
        offset + m_pos + headerSize,
        m_style->header_bg,
        m_style->radius,
        ImDrawFlags_RoundCornersTop);
    m_fullSize = m_size + paddingTL + paddingBR;
    ImU32 col = m_style->border_color;
    float thickness = m_style->border_thickness;
    ImVec2 ptl = paddingTL;
    ImVec2 pbr = paddingBR;
    if (this->isSelected()) {
        col = m_style->border_selected_color;
        thickness = m_style->border_selected_thickness;
    }
    if (thickness < 0.f) {
        ptl.x -= thickness / 2;
        ptl.y -= thickness / 2;
        pbr.x -= thickness / 2;
        pbr.y -= thickness / 2;
        thickness *= -1.f;
    }
    draw_list->AddRect(
        offset + m_pos - ptl,
        offset + m_pos + m_size + pbr,
        col,
        m_style->radius,
        0,
        thickness);

    if (ImGui::IsWindowHovered() && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ctx.mouse_on_selected_node()) {
        setSelected(false);
    }

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive() && isSelected()) {
        destroy();
    }

    if (isHovered()) {
        bool onHeader = ImGui::IsMouseHoveringRect(
            offset + m_pos - paddingTL, offset + m_pos + headerSize);

        ctx.hoveredNode(this);
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            setSelected(true);
            if (onHeader) {
                m_dragged = true;
                ctx.draggingNode(true);
            }
        }
    }

    if (m_dragged || (m_selected && ctx.isNodeDragged())) {
        float step = ctx.getStyle().grid_size / ctx.getStyle().grid_subdivisions;
        m_posTarget += ctx.getScreenSpaceDelta();
        // "Slam" The position
        m_pos.x = round(m_posTarget.x / step) * step;
        m_pos.y = round(m_posTarget.y / step) * step;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_dragged = false;
            ctx.draggingNode(false);
            m_posTarget = m_pos;
        }
    }
    ImGui::PopID();

    // Deleting dead pins
    m_dynamicIns.erase(
        std::remove_if(
            m_dynamicIns.begin(),
            m_dynamicIns.end(),
            [](const std::pair<int, std::shared_ptr<Pin>>& p) {
                return p.first == 0;
            }),
        m_dynamicIns.end());
    m_dynamicOuts.erase(
        std::remove_if(
            m_dynamicOuts.begin(),
            m_dynamicOuts.end(),
            [](const std::pair<int, std::shared_ptr<Pin>>& p) {
                return p.first == 0;
            }),
        m_dynamicOuts.end());

    m_is_hovered = ImGui::IsMouseHoveringRect(
        ctx.grid2screen(m_pos - paddingTL),
        ctx.grid2screen(m_pos + m_size + paddingBR));
}

// -----------------------------------------------------------------------------------------------------------------
// HANDLER

int ImNodeFlow::m_instances = 0;

bool ImNodeFlow::mouse_on_selected_node()
{
    return std::any_of(m_nodes.begin(), m_nodes.end(), [](const auto& n) {
        return n.second->isSelected() && n.second->isHovered();
    });
}

bool ImNodeFlow::mouse_on_free_space()
{
    return std::all_of(
               m_nodes.begin(),
               m_nodes.end(),
               [](const auto& n) { return !n.second->isHovered(); })
        && std::all_of(m_links.begin(), m_links.end(), [](const auto& l) {
               return !l->isHovered();
           });
}

ImVec2
ImNodeFlow::screen2grid(const ImVec2& p)
{
    if (ImGui::GetCurrentContext() == m_context.getRawContext())
        return p - m_context.scroll();
    return (p - m_context.origin()) / m_context.scale() - m_context.scroll();
}

ImVec2
ImNodeFlow::grid2screen(const ImVec2& p)
{
    if (ImGui::GetCurrentContext() == m_context.getRawContext())
        return p + m_context.scroll();
    return (p + m_context.scroll()) * m_context.scale() + m_context.origin();
}

void ImNodeFlow::createLink(Pin& source_pin, Pin& target_pin)
{
    if (source_pin.getType() == PinType_Input && target_pin.getType() == PinType_Output) {
        return this->createLink(target_pin, source_pin);
    }

    if (source_pin.getType() == PinType_Output && target_pin.getType() == PinType_Input) {
        if (source_pin.canCreateLink(target_pin)) {
            auto link = std::make_shared<Link>(source_pin, target_pin);
            source_pin.setLink(link);
            target_pin.setLink(link);
            m_links.emplace_back(std::move(link));
        }
    }
}

void ImNodeFlow::update()
{
    // Updating looping stuff
    m_hoveredNode = nullptr;
    m_draggingNode = m_draggingNodeNext;

    m_singleUseClicks.left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    m_singleUseClicks.right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    m_singleUseClicks.middle = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);

    // Create child canvas
    m_context.begin();
    ImGui::GetIO().IniFilename = nullptr;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Display grid
    ImVec2 gridSize = ImGui::GetWindowSize();
    float subGridStep = m_style.grid_size / m_style.grid_subdivisions;
    for (float x = fmodf(m_context.scroll().x, m_style.grid_size);
        x < gridSize.x;
        x += m_style.grid_size)
        draw_list->AddLine(
            ImVec2(x, 0.0f),
            ImVec2(x, gridSize.y),
            m_style.colors.grid);
    for (float y = fmodf(m_context.scroll().y, m_style.grid_size);
        y < gridSize.y;
        y += m_style.grid_size)
        draw_list->AddLine(
            ImVec2(0.0f, y),
            ImVec2(gridSize.x, y),
            m_style.colors.grid);
    if (m_context.scale() > 0.7f) {
        for (float x = fmodf(m_context.scroll().x, subGridStep);
            x < gridSize.x;
            x += subGridStep)
            draw_list->AddLine(
                ImVec2(x, 0.0f),
                ImVec2(x, gridSize.y),
                m_style.colors.subGrid);
        for (float y = fmodf(m_context.scroll().y, subGridStep);
            y < gridSize.y;
            y += subGridStep)
            draw_list->AddLine(
                ImVec2(0.0f, y),
                ImVec2(gridSize.x, y),
                m_style.colors.subGrid);
    }

    // Propagate pin values
    for (auto& node : m_nodes) {
        node.second->calculate();
    }
    for (auto& node : m_nodes) {
        node.second->prime();
    }

    // Update and draw nodes
    // TODO: I don't like this
    draw_list->ChannelsSplit(2);
    for (auto& node : m_nodes) {
        node.second->update(*this);
    }
    // Remove "toDelete" nodes
    for (auto iter = m_nodes.begin(); iter != m_nodes.end();) {
        if (iter->second->toDestroy())
            iter = m_nodes.erase(iter);
        else
            ++iter;
    }
    draw_list->ChannelsMerge();
    for (auto& node : m_nodes) {
        node.second->updatePublicStatus();
    }

    // Update and draw links
    for (auto& l : m_links) {
        if (!l->isMarkedForDeletion()) {
            l->update(*this);
        }
    }

    Pin* hovered_pin = nullptr;
    for (auto& elem : m_nodes) {
        BaseNode& node = *elem.second;
        for (auto& pin : node.getIns()) {
            if (pin->isHovered()) {
                hovered_pin = &*pin;
            }
        }
        for (auto& pin : node.getOuts()) {
            if (pin->isHovered()) {
                hovered_pin = &*pin;
            }
        }
        for (auto& elem : node.getDynamicIns()) {
            auto& pin = elem.second;
            if (pin->isHovered()) {
                hovered_pin = &*pin;
            }
        }
        for (auto& elem : node.getDynamicOuts()) {
            auto& pin = elem.second;
            if (pin->isHovered()) {
                hovered_pin = &*pin;
            }
        }
    }

    // Links drop-off
    if (m_dragOut && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!hovered_pin) {
            if (mouse_on_free_space() && m_droppedLinkPopUp) {
                if (m_droppedLinkPupUpComboKey == ImGuiKey_None || ImGui::IsKeyDown(m_droppedLinkPupUpComboKey)) {
                    m_droppedLinkLeft = m_dragOut;
                    ImGui::OpenPopup("DroppedLinkPopUp");
                }
            }
        } else {
            this->createLink(*m_dragOut, *hovered_pin);
        }
    }

    // Links drag-out
    if (!m_box.active && !m_draggingNode && hovered_pin && !m_dragOut && getSingleUseClick(ImGuiMouseButton_Left)) {
        consumeSingleUseClick(ImGuiMouseButton_Left);
        m_dragOut = hovered_pin;
    }

    if (m_dragOut) {
        if (m_dragOut->getType() == PinType_Output)
            detail::smart_bezier(
                m_dragOut->pinPoint(),
                ImGui::GetMousePos(),
                m_dragOut->getStyle()->color,
                m_dragOut->getStyle()
                    ->extra.link_dragged_thickness);
        else
            detail::smart_bezier(
                ImGui::GetMousePos(),
                m_dragOut->pinPoint(),
                m_dragOut->getStyle()->color,
                m_dragOut->getStyle()
                    ->extra.link_dragged_thickness);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            m_dragOut = nullptr;
    }

    if (this->mouse_on_free_space() && !m_box.active && !m_draggingNode && !m_dragOut && getSingleUseClick(getGrid().config().selection_box_button) && ImGui::IsWindowHovered()) {
        consumeSingleUseClick(getGrid().config().selection_box_button);
        m_box.active = true;
        m_box.selectionStart = ImGui::GetMousePos();

        // Deselect all nodes if not holding down CTRL
        if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
            for (auto& node : m_nodes) {
                node.second->setSelected(false);
            }
        }
    }

    if (m_box.active) {
        ImRect boxClip { m_box.selectionStart, m_box.selectionStart };
        boxClip.Add(ImGui::GetMousePos());

        draw_list->AddRectFilled(
            boxClip.GetBL(), boxClip.GetTR(), m_box.selectionColor);
        draw_list->AddRect(
            boxClip.GetBL(),
            boxClip.GetTR(),
            m_box.borderColor,
            0.0F,
            0,
            1.5F);

        bool shouldMarkSelected = false;

        if (ImGui::IsMouseReleased(
                getGrid().config().selection_box_button)) {
            m_box.active = false;
            shouldMarkSelected = true;
        } else if (
            getGrid().config().selection_box_cancel_button != getGrid().config().selection_box_button && ImGui::IsMouseClicked(getGrid().config().selection_box_cancel_button)) {
            // cancel selection
            m_box.active = false;
        }

        for (auto& elem : m_nodes) {
            auto& node = *elem.second;

            ImVec2 nodePos = grid2screen(node.getTLPos());
            ImVec2 nodeSize = node.getFullSize();

            ImRect nodeClip { nodePos, nodePos + nodeSize };
            if (boxClip.Overlaps(nodeClip)) {
                if (shouldMarkSelected) {
                    node.setSelected(true);
                } else {
                    node.setProvisionallySelected(true);
                }
            }
        }
    }

    // Right-click PopUp
    if (m_rightClickPopUp && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
        m_hoveredNodeAux = m_hoveredNode;
        ImGui::OpenPopup("RightClickPopUp");
    }
    if (ImGui::BeginPopup("RightClickPopUp")) {
        m_rightClickPopUp(m_hoveredNodeAux);
        ImGui::EndPopup();
    }

    // Dropped Link PopUp
    if (ImGui::BeginPopup("DroppedLinkPopUp")) {
        m_droppedLinkPopUp(m_droppedLinkLeft);
        ImGui::EndPopup();
    }

    // Removing dead Links
    m_links.erase(
        std::remove_if(
            m_links.begin(),
            m_links.end(),
            [](const std::shared_ptr<Link>& l) {
                return l->isMarkedForDeletion();
            }),
        m_links.end());

    if (m_context.hovered() && this->getSingleUseClick(m_context.config().scroll_button)) {
        this->consumeSingleUseClick(m_context.config().scroll_button);
        m_draggingCanvas = true;
    }

    if (m_draggingCanvas) {
        if (!ImGui::IsMouseDown(m_context.config().scroll_button)) {
            m_draggingCanvas = false;
        }
    }

    if (m_draggingCanvas && ImGui::IsMouseDragging(m_context.config().scroll_button)) {
        m_context.updateScroll();
    }

    m_context.end();
}

} // namespace ImFlow
