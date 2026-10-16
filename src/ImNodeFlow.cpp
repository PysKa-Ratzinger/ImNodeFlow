#include "ImNodeFlow.h"

#include <algorithm>
#include <cassert>
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
// NodeUID

NodeUID::NodeUID(uint64_t value) noexcept
    : id(value)
{
}

NodeUID
NodeUID::generate() noexcept
{
    static uint64_t id = 0;
    return NodeUID { id++ };
}

bool NodeUID::operator==(NodeUID const& other) const noexcept
{
    return this->id == other.id;
}

auto NodeUID::Hash::operator()(const NodeUID& uid) const -> std::size_t
{
    return std::hash<uint64_t> { }(uid.id);
}

// -----------------------------------------------------------------------------------------------------------------
// PIN

void Pin::linkTo(Pin& other)
{
    if (&m_ctx.get() != &other.m_ctx.get()) {
        // Cannot link pins across different contexts
        return;
    }

    m_ctx.get().createLink(*this, other);
}

void Pin::deleteAllLinks()
{
    auto links = std::move(m_links);

    for (auto* link : links) {
        link->destroy();
    }
}

void Pin::addLink(Link* link)
{
    auto iter = std::find(m_links.begin(), m_links.end(), link);

    if (iter != m_links.end()) {
        return;
    }

    if (m_strat == PinConnStrat_AllowOnlySingleConnection) {
        this->deleteAllLinks();
    }

    m_links.emplace_back(link);
}

void Pin::removeLink(Link* link)
{
    auto iter = std::find_if(
        m_links.begin(),
        m_links.end(),
        [link](const auto* internal_link) -> bool {
            return internal_link == link;
        });

    if (iter != m_links.end()) {
        m_links.erase(iter);
    }
}

// -----------------------------------------------------------------------------------------------------------------
// DYNAMIC PIN

DynamicPin::DynamicPin(std::unique_ptr<Pin> pin)
    : m_pin(std::move(pin))
{
    this->refresh();
}

void DynamicPin::refresh()
{
    this->m_ttl = 1;
}

void DynamicPin::update()
{
    m_ttl = m_ttl > 0 ? m_ttl - 1 : m_ttl;
    this->pin().update();
}

Pin& DynamicPin::pin()
{
    return *m_pin;
}

const Pin&
DynamicPin::pin() const
{
    return *m_pin;
}

bool DynamicPin::isStale() const
{
    return m_ttl == 0;
}

// -----------------------------------------------------------------------------------------------------------------
// LINK

Link::Link(Pin& out_pin, Pin& in_pin) noexcept
    : m_out_pin(&out_pin)
    , m_in_pin(&in_pin)
{
    assert(m_out_pin != m_in_pin && "Links cannot be created starting and ending in the same node");

    m_out_pin->addLink(this);
    m_in_pin->addLink(this);
}

Link::~Link() noexcept
{
    this->destroy();
}

void Link::destroy() noexcept
{
    if (m_out_pin != nullptr) {
        m_out_pin->removeLink(this);
        m_out_pin = nullptr;
    }
    if (m_in_pin != nullptr) {
        m_in_pin->removeLink(this);
        m_in_pin = nullptr;
    }
    m_marked_for_deletion = true;
}

void Link::update(ImNodeFlow& ctx)
{
    ImVec2 start = m_out_pin->pinPoint();
    ImVec2 end = m_in_pin->pinPoint();
    float thickness = m_out_pin->getStyle().extra.link_thickness;
    bool mouseClickState = ctx.getSingleUseClick(ImGuiMouseButton_Left);

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        m_selected = false;

    if (detail::smart_bezier_collider(
            ImGui::GetMousePos(), start, end, 2.5)) {
        m_hovered = true;
        thickness = m_out_pin->getStyle().extra.link_hovered_thickness;
        if (mouseClickState) {
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
            m_out_pin->getStyle().extra.outline_color,
            thickness + m_out_pin->getStyle().extra.link_selected_outline_thickness);
    detail::smart_bezier(start, end, m_out_pin->getStyle().color, thickness);

    if (m_selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        this->destroy();
    }
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
    auto uid = NodeUID::generate();
    auto res = m_nodes.emplace(
        uid,
        std::make_unique<Node>(*this, pos, uid, std::move(node_info)));

    assert(res.second && "Failed to create new Node");
    return *res.first->second;
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

Node::Node(ImNodeFlow& ctx, ImVec2 pos, NodeUID uid, NodeInfo node_info)
    : m_ctx(ctx)
    , m_uid(uid)
    , m_pos(pos)
    , m_posTarget(pos)
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
        m_ctx.get().grid2screen(m_pos - paddingTL),
        m_ctx.get().grid2screen(m_pos + m_size + paddingBR));
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
        pin.pin().calculate();
    }
    for (auto& pin : m_ins) {
        pin->calculate();
    }
    for (auto& pin : m_dynamicIns) {
        pin.pin().calculate();
    }
}

void Node::prime()
{
    for (auto& pin : m_outs) {
        pin->prime();
    }
    for (auto& pin : m_dynamicOuts) {
        pin.pin().prime();
    }
    for (auto& pin : m_ins) {
        pin->prime();
    }
    for (auto& pin : m_dynamicIns) {
        pin.pin().prime();
    }
}

void Node::update()
{
    auto& ctx = m_ctx.get();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::PushID(this);
    bool mouseClickState = ctx.getSingleUseClick(ImGuiMouseButton_Left);
    ImVec2 offset = ctx.grid2screen({ 0.f, 0.f });
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
        for (auto& dyn_pin : m_dynamicIns) {
            dyn_pin.pin().setPos(ImGui::GetCursorPos());
            dyn_pin.update();
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

    // Dynamic inputs stale check can only be done after the call to
    // m_impl->draw() and before any call to DynamicPin::update()
    m_dynamicIns.erase(
        std::remove_if(
            m_dynamicIns.begin(),
            m_dynamicIns.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        m_dynamicIns.end());
    m_dynamicOuts.erase(
        std::remove_if(
            m_dynamicOuts.begin(),
            m_dynamicOuts.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        m_dynamicOuts.end());

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
    for (auto& dyn_pin : m_dynamicOuts) {
        float w = dyn_pin.pin().calcWidth();
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
        p->update();
    }
    for (auto& dyn_pin : m_dynamicOuts) {
        // FIXME: This looks horrible
        if ((m_pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            dyn_pin.pin().setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - dyn_pin.pin().calcWidth(), 0.f));
        else
            dyn_pin.pin().setPos(ImVec2(
                (m_pos + ImVec2(titleW - dyn_pin.pin().calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        dyn_pin.update();
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

    if (ImGui::IsWindowHovered() && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ctx.mouse_on_selected_node())
        selected(false);

    if (isHovered()) {
        ctx.hoveredNode(this);
        if (mouseClickState) {
            selected(true);
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
        }
    }

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive() && isSelected())
        destroy();

    bool onHeader = ImGui::IsMouseHoveringRect(
        offset + m_pos - paddingTL, offset + m_pos + headerSize);
    if (onHeader && mouseClickState) {
        ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
        m_dragged = true;
        ctx.draggingNode(true);
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
        createLink(target_pin, source_pin);
        return;
    }

    if (&source_pin == &target_pin) {
        return;
    }

    if (source_pin.getType() != PinType_Output || target_pin.getType() != PinType_Input) {
        return;
    }

    bool same_node_allowed = source_pin.allowsSameNodeConnections() && target_pin.allowsSameNodeConnections();
    if (source_pin.getParent() == target_pin.getParent() && !same_node_allowed) {
        return;
    }

    for (const auto& link : source_pin.getLinks()) {
        if (&link->inPin() == &target_pin) {
            // Connection already exists
            return;
        }
    }

    // Only pins with the same type can be linked. Casts between
    // compatible types like (float and double), or (short, int, and long)
    // should be handled explicitly by user types.
    //
    // On a technical note, supporting these casts is not necessarily hard,
    // but introduces a layer of complexity which either requires RTTI (so
    // we know how to type-cast), or manual RTTI (using a compile-time
    // type_id<T>, which is possible, but we are now responsible for
    // maintaining a list of which type conversions should be possible and
    // which aren't), or it might introduce subtle programming errors
    // given the nature that different types might naturally have different
    // sizes in memory, and getting a cast wrong may result in writing
    // into out-of-bounds memory.
    //
    // Supporting connections only between same types avoids all these
    // issues, and it's not something that's prohibitibly hard to work
    // around on the client side.
    if (source_pin.getTypeID() != target_pin.getTypeID()) {
        return;
    }

    m_links.emplace_back(std::make_unique<Link>(source_pin, target_pin));
}

bool ImNodeFlow::getSingleUseClick(ImGuiMouseButton button) const
{
    switch (button) {
    case ImGuiMouseButton_Left:
        return m_singleUseClicks.left;
    case ImGuiMouseButton_Right:
        return m_singleUseClicks.right;
    case ImGuiMouseButton_Middle:
        return m_singleUseClicks.middle;
    default:
        return false;
    }
}

void ImNodeFlow::consumeSingleUseClick(ImGuiMouseButton button)
{
    switch (button) {
    case ImGuiMouseButton_Left:
        m_singleUseClicks.left = false;
        return;

    case ImGuiMouseButton_Right:
        m_singleUseClicks.right = false;
        return;

    case ImGuiMouseButton_Middle:
        m_singleUseClicks.middle = false;
        return;

    default:
        return;
    }
}

void ImNodeFlow::update()
{
    // Updating looping stuff
    m_hovering = nullptr;
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
    for (auto iter = m_links.begin(); iter != m_links.end();) {
        // Clean up deleted links along the way
        if ((*iter)->isMarkedForDeletion()) {
            iter = m_links.erase(iter);
            continue;
        }

        (*iter)->update(*this);
        ++iter;
    }

    if (m_dragOut) {
        // Links drop-off
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (m_hovering) {
                m_dragOut->linkTo(*m_hovering);
            } else if (
                mouse_on_free_space() && m_droppedLinkPopUp && (m_droppedLinkPupUpComboKey == ImGuiKey_None || ImGui::IsKeyDown(m_droppedLinkPupUpComboKey))) {
                m_droppedLinkLeft = m_dragOut;
                ImGui::OpenPopup("DroppedLinkPopUp");
            }
        }

        // Links drag-out
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

    m_context.end();
}

} // namespace ImFlow
