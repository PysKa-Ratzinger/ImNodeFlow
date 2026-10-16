#include "ImNodeFlow.h"

#include <memory>
#include <utility>

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

    return PinUID { id++ };
}

bool PinUID::operator==(PinUID const& other) const noexcept
{
    return this->id == other.id;
}

// -----------------------------------------------------------------------------------------------------------------
// LINK

void Link::update()
{
    ImVec2 start = m_out_pin->pinPoint();
    ImVec2 end = m_in_pin->pinPoint();
    float thickness = m_out_pin->getStyle().extra.link_thickness;
    bool mouseClickState = m_inf->getSingleUseClick();

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        m_selected = false;

    if (detail::smart_bezier_collider(
            ImGui::GetMousePos(), start, end, 2.5)) {
        m_hovered = true;
        thickness = m_out_pin->getStyle().extra.link_hovered_thickness;
        if (mouseClickState) {
            m_inf->consumeSingleUseClick();
            m_selected = true;
        }
    } else {
        m_hovered = false;
    }

    if (m_selected)
        detail::smart_bezier(
            start,
            end,
            m_out_pin->getStyle().extra.outline_color,
            thickness + m_out_pin->getStyle().extra.link_selected_outline_thickness);
    detail::smart_bezier(start, end, m_out_pin->getStyle().color, thickness);

    if (m_selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        m_in_pin->deleteLink();
}

Link::~Link()
{
    if (!m_out_pin)
        return;
    m_out_pin->deleteLink();
}

// -----------------------------------------------------------------------------------------------------------------
// FLOW CONTEXT

namespace {

    /**
     * @brief Helper struct for creating a node from a lambda
     */
    struct NodeWrapper : public INodeBehaviour {
        std::function<void(Node&)> m_draw_func;

        NodeWrapper(std::function<void(Node&)> draw_func)
            : m_draw_func(std::move(draw_func))
        {
        }

        void draw(Node& node) override { m_draw_func(node); }
    };

} // anonymous namespace

Node& ImNodeFlow::addNode(const ImVec2& pos, NodeInfo node_info)
{
    auto node = std::make_unique<Node>(
        pos, this, NodeUID { 0 }, std::move(node_info));
    auto* node_ptr = node.get();
    auto uid = reinterpret_cast<uintptr_t>(node_ptr);
    node->setUID(uid);
    m_nodes[uid] = std::move(node);
    return *node_ptr;
}

Node& ImNodeFlow::placeNodeAt(const ImVec2& pos, NodeInfo node_info)
{
    return addNode(screen2grid(pos), std::move(node_info));
}

Node& ImNodeFlow::placeNode(NodeInfo node_info)
{
    return placeNodeAt(ImGui::GetMousePos(), std::move(node_info));
}

Node& ImNodeFlow::addNodeLambda(
    const ImVec2& pos,
    std::function<void(Node&)> draw_func)
{
    return addNode(
        pos,
        NodeInfo {
            .behaviour = std::make_unique<NodeWrapper>(std::move(draw_func)),
        });
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

Node::Node(ImVec2 pos, ImNodeFlow* inf, NodeUID uid, NodeInfo node_info)
    : m_uid(uid)
    , m_pos(pos)
    , m_posTarget(pos)
    , m_inf(inf)
    , m_style(std::move(node_info.style))
    , m_impl(std::move(node_info.behaviour))
{
    if (m_impl) {
        m_impl->setup(*this);
    }
}

void Node::destroy()
{
    if (m_impl) {
        m_impl.reset();
    }
    m_destroyed = true;
}

bool Node::isHovered()
{
    ImVec2 paddingTL = { m_style.padding.x, m_style.padding.y };
    ImVec2 paddingBR = { m_style.padding.z, m_style.padding.w };
    return ImGui::IsMouseHoveringRect(
        m_inf->grid2screen(m_pos - paddingTL),
        m_inf->grid2screen(m_pos + m_size + paddingBR));
}

void Node::dropIN(const PinUID& uid)
{
    auto iter = std::find_if(m_ins.begin(), m_ins.end(), [&](auto pin) -> bool {
        return pin->getUid() == uid;
    });

    if (iter != m_ins.end()) {
        m_ins.erase(iter);
    }
}

void Node::dropIN(const char* name)
{
    m_ins.erase(
        std::remove_if(
            m_ins.begin(),
            m_ins.end(),
            [&](auto pin) -> bool { return pin->getName() == name; }),
        m_ins.end());
}

void Node::dropOUT(const PinUID& uid)
{
    auto iter = std::find_if(
        m_outs.begin(), m_outs.end(), [&uid](auto const& pin) -> bool {
            return pin->getUid() == uid;
        });

    if (iter != m_outs.end()) {
        m_outs.erase(iter);
    }
}

inline void
Node::dropOUT(const char* name)
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

Pin* Node::inPin(const PinUID& uid)
{
    auto it = std::find_if(m_ins.begin(), m_ins.end(), [&uid](const auto& p) {
        return p->getUid() == uid;
    });
    return it == m_ins.end() ? nullptr : &**it;
}

Pin* Node::inPin(const char* name)
{
    auto it = std::find_if(
        m_ins.begin(), m_ins.end(), [&name](const auto& p) {
            return p->getName() == name;
        });
    return it == m_ins.end() ? nullptr : &**it;
}

Pin* Node::outPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_outs.begin(), m_outs.end(), [&uid](const auto& p) {
            return p->getUid() == uid;
        });
    return it == m_outs.end() ? nullptr : &**it;
}

Pin* Node::outPin(const char* name)
{
    auto it = std::find_if(
        m_outs.begin(), m_outs.end(), [&name](const auto& p) {
            return p->getName() == name;
        });
    return it == m_outs.end() ? nullptr : &**it;
}

void Node::calculate()
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

void Node::prime()
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

void Node::update()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::PushID(this);
    bool mouseClickState = m_inf->getSingleUseClick();
    ImVec2 offset = m_inf->grid2screen({ 0.f, 0.f });
    ImVec2 paddingTL = { m_style.padding.x, m_style.padding.y };
    ImVec2 paddingBR = { m_style.padding.z, m_style.padding.w };

    draw_list->ChannelsSetCurrent(1); // Foreground
    ImGui::SetCursorScreenPos(offset + m_pos);

    ImGui::BeginGroup();

    // Header
    ImGui::BeginGroup();
    ImGui::TextColored(m_style.header_title_color, "%s", m_title.c_str());
    ImGui::Spacing();
    ImGui::EndGroup();
    float headerH = ImGui::GetItemRectSize().y;
    float titleW = ImGui::GetItemRectSize().x;

    // Inputs
    if (!m_ins.empty() || !m_dynamicIns.empty()) {
        ImGui::BeginGroup();
        for (auto& p : m_ins) {
            p->setPos(ImGui::GetCursorPos());
            p->update();
        }
        for (auto& p : m_dynamicIns) {
            if (p.first == 1) {
                p.second->setPos(ImGui::GetCursorPos());
                p.second->update();
                p.first = 0;
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();
    }

    // Content
    ImGui::BeginGroup();

    if (m_impl) {
        m_impl->draw(*this);
    } else {
        ImGui::TextUnformatted("<no renderer provided>");
    }

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
        if ((m_pos + ImVec2(titleW, 0) + m_inf->getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            p->setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p->calcWidth(), 0.f));
        else
            p->setPos(ImVec2(
                (m_pos + ImVec2(titleW - p->calcWidth(), 0) + m_inf->getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        p->update();
    }
    for (auto& p : m_dynamicOuts) {
        // FIXME: This looks horrible
        if ((m_pos + ImVec2(titleW, 0) + m_inf->getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            p.second->setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p.second->calcWidth(), 0.f));
        else
            p.second->setPos(ImVec2(
                (m_pos + ImVec2(titleW - p.second->calcWidth(), 0) + m_inf->getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        p.second->update();
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
        m_style.bg,
        m_style.radius);
    draw_list->AddRectFilled(
        offset + m_pos - paddingTL,
        offset + m_pos + headerSize,
        m_style.header_bg,
        m_style.radius,
        ImDrawFlags_RoundCornersTop);
    m_fullSize = m_size + paddingTL + paddingBR;
    ImU32 col = m_style.border_color;
    float thickness = m_style.border_thickness;
    ImVec2 ptl = paddingTL;
    ImVec2 pbr = paddingBR;
    if (m_selected) {
        col = m_style.border_selected_color;
        thickness = m_style.border_selected_thickness;
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
        m_style.radius,
        0,
        thickness);

    if (ImGui::IsWindowHovered() && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_inf->mouse_on_selected_node())
        selected(false);

    if (isHovered()) {
        m_inf->hoveredNode(this);
        if (mouseClickState) {
            selected(true);
            m_inf->consumeSingleUseClick();
        }
    }

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive() && isSelected())
        destroy();

    bool onHeader = ImGui::IsMouseHoveringRect(
        offset + m_pos - paddingTL, offset + m_pos + headerSize);
    if (onHeader && mouseClickState) {
        m_inf->consumeSingleUseClick();
        m_dragged = true;
        m_inf->draggingNode(true);
    }
    if (m_dragged || (m_selected && m_inf->isNodeDragged())) {
        float step = m_inf->getStyle().grid_size / m_inf->getStyle().grid_subdivisions;
        m_posTarget += m_inf->getScreenSpaceDelta();
        // "Slam" The position
        m_pos.x = round(m_posTarget.x / step) * step;
        m_pos.y = round(m_posTarget.y / step) * step;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_dragged = false;
            m_inf->draggingNode(false);
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
               return !l.lock()->isHovered();
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

void ImNodeFlow::addLink(std::shared_ptr<Link>& link)
{
    m_links.push_back(link);
}

void ImNodeFlow::update()
{
    // Updating looping stuff
    m_hovering = nullptr;
    m_hoveredNode = nullptr;
    m_draggingNode = m_draggingNodeNext;
    m_singleUseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

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
        node.second->update();
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
        if (!l.expired())
            l.lock()->update();
    }

    // Links drop-off
    if (m_dragOut && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!m_hovering) {
            if (mouse_on_free_space() && m_droppedLinkPopUp) {
                if (m_droppedLinkPupUpComboKey == ImGuiKey_None || ImGui::IsKeyDown(m_droppedLinkPupUpComboKey)) {
                    m_droppedLinkLeft = m_dragOut;
                    ImGui::OpenPopup("DroppedLinkPopUp");
                }
            }
        } else
            m_dragOut->createLink(m_hovering);
    }

    // Links drag-out
    if (!m_draggingNode && m_hovering && !m_dragOut && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        m_dragOut = m_hovering;
    if (m_dragOut) {
        if (m_dragOut->getType() == PinType_Output)
            detail::smart_bezier(
                m_dragOut->pinPoint(),
                ImGui::GetMousePos(),
                m_dragOut->getStyle().color,
                m_dragOut->getStyle().extra.link_dragged_thickness);
        else
            detail::smart_bezier(
                ImGui::GetMousePos(),
                m_dragOut->pinPoint(),
                m_dragOut->getStyle().color,
                m_dragOut->getStyle().extra.link_dragged_thickness);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            m_dragOut = nullptr;
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
            [](const std::weak_ptr<Link>& l) { return l.expired(); }),
        m_links.end());

    m_context.end();
}

} // namespace ImFlow
