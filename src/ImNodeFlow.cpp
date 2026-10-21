#include "ImNodeFlow.h"

#include "ImNodeFlow_internals.h"

#include "imgui_bezier_items.h"

#include <imgui_internal.h>

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

void Pin::update(bool* out_hovering)
{
    if (!m_renderer) {
        ImGui::SetCursorPos(m_pos);
        ImGui::Text("%s", m_name.c_str());
    } else {
        ImGui::BeginGroup();
        m_renderer(this);
        ImGui::EndGroup();
    }

    m_size = ImGui::GetItemRectSize();

    bool is_hovering = false;

    if (!m_renderer) {
        drawDecoration();
        drawSocket(&is_hovering);
    }

    // Calculate the combined hitbox if activated
    bool item_hovered = ImGui::IsItemHovered();
    bool socket_hovered = false;

    if (m_socketHitboxEnabled) {
        auto [socket_tl, socket_br] = getSocketHitBounds();
        socket_hovered = ImGui::IsMouseHoveringRect(socket_tl, socket_br);
    }

    if (item_hovered || socket_hovered) {
        is_hovering = true;
    }

    if (out_hovering != nullptr) {
        *out_hovering = is_hovering;
    }
}

void Pin::drawSocket(bool* out_hovering_socket)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto [tl, br] = getSocketHitBounds();

    // Determine if we are in hover (checked now in update())
    bool socketHovered = ImGui::IsMouseHoveringRect(tl, br);

    if (isConnected())
        draw_list->AddCircleFilled(
            pinPoint(),
            m_style.socket_connected_radius,
            m_style.color,
            m_style.socket_shape);
    else {
        if (socketHovered) {
            draw_list->AddCircle(
                pinPoint(),
                m_style.socket_hovered_radius,
                m_style.color,
                m_style.socket_shape,
                m_style.socket_thickness);
        } else {
            draw_list->AddCircle(
                pinPoint(),
                m_style.socket_radius,
                m_style.color,
                m_style.socket_shape,
                m_style.socket_thickness);
        }
    }

    if (out_hovering_socket != nullptr) {
        *out_hovering_socket = socketHovered;
    }
}

inline void
Pin::drawDecoration()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Check hover on text OR socket
    bool itemHovered = ImGui::IsItemHovered();
    bool socketHovered = false;

    if (m_socketHitboxEnabled) {
        auto [socket_tl, socket_br] = getSocketHitBounds();
        socketHovered = ImGui::IsMouseHoveringRect(socket_tl, socket_br);
    }

    if (itemHovered || socketHovered) {
        draw_list->AddRectFilled(
            m_pos - m_style.extra.padding,
            m_pos + m_size + m_style.extra.padding,
            m_style.extra.bg_hover_color,
            m_style.extra.bg_radius);
    } else {
        draw_list->AddRectFilled(
            m_pos - m_style.extra.padding,
            m_pos + m_size + m_style.extra.padding,
            m_style.extra.bg_color,
            m_style.extra.bg_radius);
    }

    draw_list->AddRect(
        m_pos - m_style.extra.padding,
        m_pos + m_size + m_style.extra.padding,
        m_style.extra.border_color,
        m_style.extra.bg_radius,
        0,
        m_style.extra.border_thickness);
}

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

inline std::pair<ImVec2, ImVec2>
Pin::getSocketHitBounds(float expand_radius)
{
    if (expand_radius < 0.0f) {
        expand_radius = m_style.socket_hovered_radius;
    }

    ImVec2 center = pinPoint();
    ImVec2 tl = center - ImVec2(expand_radius, expand_radius);
    ImVec2 br = center + ImVec2(expand_radius, expand_radius);

    return { tl, br };
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

    if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        m_selected = false;

    if (detail::smart_bezier_collider(
            ImGui::GetMousePos(), start, end, 2.5)) {
        m_hovered = true;
        thickness = m_out_pin->getStyle().extra.link_hovered_thickness;
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
            m_out_pin->getStyle().extra.outline_color,
            thickness + m_out_pin->getStyle().extra.link_selected_outline_thickness);
    detail::smart_bezier_gradient(
        start,
        end,
        m_out_pin->getStyle().color,
        m_in_pin->getStyle().color,
        thickness,
        ctx.getStyle().link_gradient_blend);

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

Node& ImNodeFlow::addNode(const ImVec2& grid_coords, NodeInfo node_info)
{
    auto uid = NodeUID::generate();
    auto res = m_impl->nodes.emplace(
        uid,
        std::make_unique<Node>(
            *this, grid_coords, uid, std::move(node_info)));

    assert(res.second && "Failed to create new Node");
    return *res.first->second;
}

Node& ImNodeFlow::placeNodeAt(const ImVec2& screen_coords, NodeInfo node_info)
{
    return addNode(screen2grid(screen_coords), std::move(node_info));
}

Node& ImNodeFlow::placeNode(NodeInfo node_info)
{
    return placeNodeAt(ImGui::GetMousePos(), std::move(node_info));
}

Node& ImNodeFlow::addNodeLambda(
    const ImVec2& grid_coords,
    std::function<void(Node&)> draw_func)
{
    return addNode(
        grid_coords,
        NodeInfo {
            .behaviour = std::make_unique<NodeWrapper>(std::move(draw_func)),
        });
}

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

Node::Node(ImNodeFlow& ctx, ImVec2 pos, NodeUID uid, NodeInfo node_info)
    : m_impl(std::make_unique<Node::impl_t>(Node::impl_t {
          .ctx { ctx },
          .uid { uid },
          .pos { pos },
          .posTarget { pos },
          .style { std::move(node_info.style) },
          .behaviour { std::move(node_info.behaviour) },
      }))
{
    if (m_impl->behaviour) {
        m_impl->behaviour->setup(*this);
    }
}

void Node::destroy()
{
    if (m_impl->behaviour) {
        m_impl->behaviour.reset();
    }
    m_impl->destroyed = true;
}

bool Node::toDestroy() const
{
    return m_impl->destroyed;
}

bool Node::isHovered()
{
    ImVec2 paddingTL = { m_impl->style.padding.x, m_impl->style.padding.y };
    ImVec2 paddingBR = { m_impl->style.padding.z, m_impl->style.padding.w };
    return ImGui::IsMouseHoveringRect(
        m_impl->ctx.get().grid2screen(m_impl->pos - paddingTL),
        m_impl->ctx.get().grid2screen(m_impl->pos + m_impl->size + paddingBR));
}

NodeUID Node::getUID() const
{
    return m_impl->uid;
}

const std::string& Node::getName() const
{
    return m_impl->title;
}

const ImVec2& Node::getSize() const
{
    return m_impl->size;
}

const ImVec2& Node::getFullSize() const
{
    return m_impl->fullSize;
}

const ImVec2& Node::getPos() const
{
    return m_impl->pos;
}

ImVec2 Node::getTLPos() const
{
    return m_impl->pos - ImVec2 { m_impl->style.padding.x, m_impl->style.padding.y };
}

const NodeStyle& Node::getStyle() const
{
    return m_impl->style;
}

bool Node::isSelected() const
{
    return m_impl->selected || m_impl->provisionallySelected;
}

bool Node::isDragged() const
{
    return m_impl->dragged;
}

bool Node::isIndestructable() const
{
    return m_impl->indestructable;
}

Node& Node::setUID(NodeUID uid)
{
    m_impl->uid = uid;
    return *this;
}

Node& Node::setTitle(const std::string& title)
{
    m_impl->title = title;
    return *this;
}

Node& Node::setPos(const ImVec2& pos)
{
    m_impl->pos = pos;
    m_impl->posTarget = pos;
    return *this;
}

Node& Node::setStyle(NodeStyle style)
{
    m_impl->style = std::move(style);
    return *this;
}

Node& Node::setIndestructable(bool indestruct)
{
    m_impl->indestructable = indestruct;
    return *this;
}

Node& Node::setSelected(bool state)
{
    m_impl->selectedNext = state;
    return *this;
}

Node& Node::setProvisionallySelected(bool state)
{
    m_impl->provisionallySelectedNext = state;
    return *this;
}

void Node::updatePublicStatus()
{
    m_impl->selected = m_impl->selectedNext;
    m_impl->provisionallySelected = m_impl->provisionallySelectedNext;
    m_impl->provisionallySelectedNext = false;
}

void Node::dropIN(const PinUID& uid)
{
    auto iter = std::find_if(
        m_impl->ins.begin(), m_impl->ins.end(), [&](auto const& pin) -> bool {
            return pin->getUid() == uid;
        });

    if (iter != m_impl->ins.end()) {
        m_impl->ins.erase(iter);
    }
}

void Node::dropIN(const char* name)
{
    m_impl->ins.erase(
        std::remove_if(
            m_impl->ins.begin(),
            m_impl->ins.end(),
            [&](auto const& pin) -> bool {
                return pin->getName() == name;
            }),
        m_impl->ins.end());
}

void Node::dropOUT(const PinUID& uid)
{
    auto iter = std::find_if(
        m_impl->outs.begin(),
        m_impl->outs.end(),
        [&uid](auto const& pin) -> bool {
            return pin->getUid() == uid;
        });

    if (iter != m_impl->outs.end()) {
        m_impl->outs.erase(iter);
    }
}

inline void
Node::dropOUT(const char* name)
{
    m_impl->outs.erase(
        std::remove_if(
            m_impl->outs.begin(),
            m_impl->outs.end(),
            [&name](auto const& pin) -> bool {
                return pin->getName() == name;
            }),
        m_impl->outs.end());
}

Pin* Node::inPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_impl->ins.begin(),
        m_impl->ins.end(),
        [&uid](const auto& p) {
            return p->getUid() == uid;
        });
    return it == m_impl->ins.end() ? nullptr : &**it;
}

Pin* Node::inPin(const char* name)
{
    auto it = std::find_if(
        m_impl->ins.begin(),
        m_impl->ins.end(),
        [&name](const auto& p) {
            return p->getName() == name;
        });
    return it == m_impl->ins.end() ? nullptr : &**it;
}

Pin* Node::outPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_impl->outs.begin(),
        m_impl->outs.end(),
        [&uid](const auto& p) {
            return p->getUid() == uid;
        });
    return it == m_impl->outs.end() ? nullptr : &**it;
}

Pin* Node::outPin(const char* name)
{
    auto it = std::find_if(
        m_impl->outs.begin(),
        m_impl->outs.end(),
        [&name](const auto& p) {
            return p->getName() == name;
        });
    return it == m_impl->outs.end() ? nullptr : &**it;
}

void Node::calculate()
{
    for (auto& pin : m_impl->outs) {
        pin->calculate();
    }
    for (auto& pin : m_impl->dynamicOuts) {
        pin.pin().calculate();
    }
    for (auto& pin : m_impl->ins) {
        pin->calculate();
    }
    for (auto& pin : m_impl->dynamicIns) {
        pin.pin().calculate();
    }
}

void Node::prime()
{
    for (auto& pin : m_impl->outs) {
        pin->prime();
    }
    for (auto& pin : m_impl->dynamicOuts) {
        pin.pin().prime();
    }
    for (auto& pin : m_impl->ins) {
        pin->prime();
    }
    for (auto& pin : m_impl->dynamicIns) {
        pin.pin().prime();
    }
}

void Node::update()
{
    auto& ctx = m_impl->ctx.get();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::PushID(this);
    ImVec2 offset = ctx.grid2screen({ 0.f, 0.f });
    ImVec2 paddingTL = { m_impl->style.padding.x, m_impl->style.padding.y };
    ImVec2 paddingBR = { m_impl->style.padding.z, m_impl->style.padding.w };

    draw_list->ChannelsSetCurrent(1); // Foreground
    ImGui::SetCursorScreenPos(offset + m_impl->pos);

    ImGui::BeginGroup();

    // Header
    ImGui::BeginGroup();
    ImGui::TextColored(m_impl->style.header_title_color, "%s", m_impl->title.c_str());
    ImGui::Spacing();
    ImGui::EndGroup();
    float headerH = ImGui::GetItemRectSize().y;
    float titleW = ImGui::GetItemRectSize().x;

    Pin* hovered_pin = nullptr;

    // Inputs
    if (!m_impl->ins.empty() || !m_impl->dynamicIns.empty()) {
        ImGui::BeginGroup();
        for (auto& p : m_impl->ins) {
            p->setPos(ImGui::GetCursorPos());
            bool is_hovering = false;
            p->update(&is_hovering);
            if (is_hovering && hovered_pin == nullptr) {
                hovered_pin = &*p;
            }
        }
        for (auto& dyn_pin : m_impl->dynamicIns) {
            dyn_pin.pin().setPos(ImGui::GetCursorPos());
            bool is_hovering = false;
            dyn_pin.update(&is_hovering);
            if (is_hovering && hovered_pin == nullptr) {
                hovered_pin = &dyn_pin.pin();
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();
    }

    // Content
    ImGui::BeginGroup();

    if (m_impl->behaviour) {
        m_impl->behaviour->draw(*this);
    } else {
        ImGui::TextUnformatted("<no renderer provided>");
    }

    // Dynamic inputs stale check can only be done after the call to
    // behaviour->draw() and before any call to DynamicPin::update()
    m_impl->dynamicIns.erase(
        std::remove_if(
            m_impl->dynamicIns.begin(),
            m_impl->dynamicIns.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        m_impl->dynamicIns.end());
    m_impl->dynamicOuts.erase(
        std::remove_if(
            m_impl->dynamicOuts.begin(),
            m_impl->dynamicOuts.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        m_impl->dynamicOuts.end());

    ImGui::Dummy(ImVec2(0.f, 0.f));
    ImGui::EndGroup();
    ImGui::SameLine();

    // Outputs
    float maxW = 0.0f;
    for (auto& p : m_impl->outs) {
        float w = p->calcWidth();
        if (w > maxW)
            maxW = w;
    }
    for (auto& dyn_pin : m_impl->dynamicOuts) {
        float w = dyn_pin.pin().calcWidth();
        if (w > maxW)
            maxW = w;
    }
    ImGui::BeginGroup();
    for (auto& p : m_impl->outs) {
        // FIXME: This looks horrible
        if ((m_impl->pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            p->setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p->calcWidth(), 0.f));
        else
            p->setPos(ImVec2(
                (m_impl->pos + ImVec2(titleW - p->calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        bool is_hovering = false;
        p->update(&is_hovering);
        if (is_hovering && hovered_pin == nullptr) {
            hovered_pin = &*p;
        }
    }
    for (auto& dyn_pin : m_impl->dynamicOuts) {
        // FIXME: This looks horrible
        if ((m_impl->pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            dyn_pin.pin().setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - dyn_pin.pin().calcWidth(), 0.f));
        else
            dyn_pin.pin().setPos(ImVec2(
                (m_impl->pos + ImVec2(titleW - dyn_pin.pin().calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        bool is_hovering = false;
        dyn_pin.update(&is_hovering);
        if (is_hovering && hovered_pin == nullptr) {
            hovered_pin = &dyn_pin.pin();
        }
    }

    ImGui::EndGroup();

    ImGui::EndGroup();
    m_impl->size = ImGui::GetItemRectSize();
    ImVec2 headerSize = ImVec2(m_impl->size.x + paddingBR.x, headerH);

    // Background
    draw_list->ChannelsSetCurrent(0);
    draw_list->AddRectFilled(
        offset + m_impl->pos - paddingTL,
        offset + m_impl->pos + m_impl->size + paddingBR,
        m_impl->style.bg,
        m_impl->style.radius);
    draw_list->AddRectFilled(
        offset + m_impl->pos - paddingTL,
        offset + m_impl->pos + headerSize,
        m_impl->style.header_bg,
        m_impl->style.radius,
        ImDrawFlags_RoundCornersTop);
    m_impl->fullSize = m_impl->size + paddingTL + paddingBR;
    ImU32 col = m_impl->style.border_color;
    float thickness = m_impl->style.border_thickness;
    ImVec2 ptl = paddingTL;
    ImVec2 pbr = paddingBR;
    if (this->isSelected()) {
        col = m_impl->style.border_selected_color;
        thickness = m_impl->style.border_selected_thickness;
    }
    if (thickness < 0.f) {
        ptl.x -= thickness / 2;
        ptl.y -= thickness / 2;
        pbr.x -= thickness / 2;
        pbr.y -= thickness / 2;
        thickness *= -1.f;
    }
    draw_list->AddRect(
        offset + m_impl->pos - ptl,
        offset + m_impl->pos + m_impl->size + pbr,
        col,
        m_impl->style.radius,
        0,
        thickness);

    if (hovered_pin != nullptr) {
        ctx.hovering(hovered_pin);
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            ctx.dragging(hovered_pin);
        }
    }

    if (ImGui::IsWindowHovered() && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ctx.mouse_on_selected_node()) {
        setSelected(false);
    }

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive() && isSelected() && !isIndestructable()) {
        destroy();
    }

    if (isHovered()) {
        bool onHeader = ImGui::IsMouseHoveringRect(
            offset + m_impl->pos - paddingTL, offset + m_impl->pos + headerSize);

        ctx.hoveredNode(this);
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            setSelected(true);
            if (onHeader) {
                m_impl->dragged = true;
                ctx.draggingNode(true);
            }
        }
    }

    if (m_impl->dragged || (m_impl->selected && ctx.isNodeDragged())) {
        float step = ctx.getStyle().grid_size / ctx.getStyle().grid_subdivisions;
        m_impl->posTarget += ctx.getScreenSpaceDelta();
        // "Slam" The position
        m_impl->pos.x = round(m_impl->posTarget.x / step) * step;
        m_impl->pos.y = round(m_impl->posTarget.y / step) * step;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_impl->dragged = false;
            ctx.draggingNode(false);
            m_impl->posTarget = m_impl->pos;
        }
    }
    ImGui::PopID();
}

// -----------------------------------------------------------------------------------------------------------------
// HANDLER

int ImNodeFlow::m_instances = 0;

ImNodeFlow::ImNodeFlow()
    : ImNodeFlow("FlowGrid" + std::to_string(m_instances))
{
}

ImNodeFlow::ImNodeFlow(std::string name)
    : m_impl(std::make_unique<detail::editor_context_t>(
          detail::editor_context_t {
              .name { std::move(name) },
          }))
{
    m_instances++;
    m_impl->context.config().extra_window_wrapper = true;
    m_impl->context.config().color = m_impl->style.colors.background;
}

ImNodeFlow::ImNodeFlow(ImNodeFlow&&) noexcept = default;
ImNodeFlow& ImNodeFlow::operator=(ImNodeFlow&&) = default;

ImNodeFlow::~ImNodeFlow() = default;

bool ImNodeFlow::mouse_on_selected_node()
{
    return std::any_of(m_impl->nodes.begin(), m_impl->nodes.end(), [](const auto& n) {
        return n.second->isSelected() && n.second->isHovered();
    });
}

bool ImNodeFlow::mouse_on_free_space()
{
    return std::all_of(
               m_impl->nodes.begin(),
               m_impl->nodes.end(),
               [](const auto& n) { return !n.second->isHovered(); })
        && std::all_of(m_impl->links.begin(), m_impl->links.end(), [](const auto& l) {
               return !l->isHovered();
           });
}

ImVec2
ImNodeFlow::screen2grid(const ImVec2& p)
{
    if (ImGui::GetCurrentContext() == m_impl->context.getRawContext())
        return p - m_impl->context.scroll();
    return (p - m_impl->context.origin()) / m_impl->context.scale() - m_impl->context.scroll();
}

ImVec2
ImNodeFlow::grid2screen(const ImVec2& p)
{
    if (ImGui::GetCurrentContext() == m_impl->context.getRawContext())
        return p + m_impl->context.scroll();
    return (p + m_impl->context.scroll()) * m_impl->context.scale() + m_impl->context.origin();
}

void ImNodeFlow::createLink(Pin& source_pin, Pin& target_pin)
{
    if (source_pin.getType() == PinType_Input
        && target_pin.getType() == PinType_Output) {
        createLink(target_pin, source_pin);
        return;
    }

    if (&source_pin == &target_pin) {
        return;
    }

    if (source_pin.getType() != PinType_Output
        || target_pin.getType() != PinType_Input) {
        return;
    }

    bool same_node_allowed = source_pin.allowsSameNodeConnections()
        && target_pin.allowsSameNodeConnections();
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

    m_impl->links.emplace_back(std::make_unique<Link>(source_pin, target_pin));
}

void ImNodeFlow::droppedLinkPopUpContent(
    std::function<void(Pin* dragged)> content,
    ImGuiKey key)
{
    m_impl->droppedLinkPopUp = std::move(content);
    m_impl->droppedLinkPupUpComboKey = key;
}

void ImNodeFlow::rightClickPopUpContent(right_click_popup_handler_t content)
{
    m_impl->rightClickPopUp = std::move(content);
}

void ImNodeFlow::dragDropTarget(std::function<void(const ImVec2&)> content)
{
    m_impl->dragdropTarget = std::move(content);
}

bool ImNodeFlow::getSingleUseClick(ImGuiMouseButton button) const
{
    switch (button) {
    case ImGuiMouseButton_Left:
        return m_impl->singleUseClicks.left;
    case ImGuiMouseButton_Right:
        return m_impl->singleUseClicks.right;
    case ImGuiMouseButton_Middle:
        return m_impl->singleUseClicks.middle;
    default:
        return false;
    }
}

void ImNodeFlow::consumeSingleUseClick(ImGuiMouseButton button)
{
    switch (button) {
    case ImGuiMouseButton_Left:
        m_impl->singleUseClicks.left = false;
        return;

    case ImGuiMouseButton_Right:
        m_impl->singleUseClicks.right = false;
        return;

    case ImGuiMouseButton_Middle:
        m_impl->singleUseClicks.middle = false;
        return;

    default:
        return;
    }
}

const std::string& ImNodeFlow::getName() const
{
    return m_impl->name;
}

const ImVec2& ImNodeFlow::getPos() const
{
    return m_impl->context.origin();
}

const ImVec2& ImNodeFlow::getScroll() const
{
    return m_impl->context.scroll();
}

void ImNodeFlow::setScroll(ImVec2 scroll)
{
    m_impl->context.setScroll(scroll);
}

void ImNodeFlow::setScale(float scale)
{
    m_impl->context.setScale(scale);
}

ImVec2 ImNodeFlow::getScreenSpaceDelta() const
{
    return m_impl->context.getScreenDelta();
}

uint64_t ImNodeFlow::getNodesCount() const
{
    return m_impl->nodes.size();
}

ContainedContext& ImNodeFlow::getGrid()
{
    return m_impl->context;
}

[[nodiscard]] bool ImNodeFlow::isNodeDragged() const
{
    return m_impl->draggingNode;
}

const InfStyler& ImNodeFlow::getStyle() const
{
    return m_impl->style;
}

void ImNodeFlow::setStyle(InfStyler style)
{
    m_impl->style = std::move(style);
}

void ImNodeFlow::setSize(const ImVec2& size)
{
    m_impl->context.config().size = size;
}

void ImNodeFlow::draggingNode(bool state)
{
    m_impl->draggingNodeNext = state;
}

void ImNodeFlow::hovering(Pin* hovering)
{
    m_impl->hovering = hovering;
}

void ImNodeFlow::dragging(Pin* dragged)
{
    m_impl->dragOut = dragged;
}

void ImNodeFlow::hoveredNode(Node* hovering)
{
    m_impl->hoveredNode = hovering;
}

void ImNodeFlow::update()
{
    // Updating looping stuff
    m_impl->hovering = nullptr;
    m_impl->hoveredNode = nullptr;
    m_impl->draggingNode = m_impl->draggingNodeNext;

    m_impl->singleUseClicks.left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    m_impl->singleUseClicks.right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    m_impl->singleUseClicks.middle = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);

    // Create child canvas
    m_impl->context.begin();
    ImGui::GetIO().IniFilename = nullptr;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Display grid
    ImVec2 gridSize = ImGui::GetWindowSize();
    float subGridStep = m_impl->style.grid_size / m_impl->style.grid_subdivisions;
    for (float x = fmodf(m_impl->context.scroll().x, m_impl->style.grid_size);
        x < gridSize.x;
        x += m_impl->style.grid_size)
        draw_list->AddLine(
            ImVec2(x, 0.0f),
            ImVec2(x, gridSize.y),
            m_impl->style.colors.grid);
    for (float y = fmodf(m_impl->context.scroll().y, m_impl->style.grid_size);
        y < gridSize.y;
        y += m_impl->style.grid_size)
        draw_list->AddLine(
            ImVec2(0.0f, y),
            ImVec2(gridSize.x, y),
            m_impl->style.colors.grid);
    if (m_impl->context.scale() > 0.7f) {
        for (float x = fmodf(m_impl->context.scroll().x, subGridStep);
            x < gridSize.x;
            x += subGridStep)
            draw_list->AddLine(
                ImVec2(x, 0.0f),
                ImVec2(x, gridSize.y),
                m_impl->style.colors.subGrid);
        for (float y = fmodf(m_impl->context.scroll().y, subGridStep);
            y < gridSize.y;
            y += subGridStep)
            draw_list->AddLine(
                ImVec2(0.0f, y),
                ImVec2(gridSize.x, y),
                m_impl->style.colors.subGrid);
    }

    // Propagate pin values
    for (auto& node : m_impl->nodes) {
        node.second->calculate();
    }
    for (auto& node : m_impl->nodes) {
        node.second->prime();
    }

    // Update and draw nodes
    // TODO: I don't like this
    draw_list->ChannelsSplit(2);
    for (auto& node : m_impl->nodes) {
        node.second->update();
    }
    // Remove "toDelete" nodes
    for (auto iter = m_impl->nodes.begin(); iter != m_impl->nodes.end();) {
        if (iter->second->toDestroy())
            iter = m_impl->nodes.erase(iter);
        else
            ++iter;
    }
    draw_list->ChannelsMerge();
    for (auto& node : m_impl->nodes) {
        node.second->updatePublicStatus();
    }

    // Update and draw links
    for (auto iter = m_impl->links.begin(); iter != m_impl->links.end();) {
        // Clean up deleted links along the way
        if ((*iter)->isMarkedForDeletion()) {
            iter = m_impl->links.erase(iter);
            continue;
        }

        (*iter)->update(*this);
        ++iter;
    }

    if (m_impl->dragOut) {
        // Links drop-off
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (m_impl->hovering) {
                m_impl->dragOut->linkTo(*m_impl->hovering);
            } else if (
                mouse_on_free_space() && m_impl->droppedLinkPopUp && (m_impl->droppedLinkPupUpComboKey == ImGuiKey_None || ImGui::IsKeyDown(m_impl->droppedLinkPupUpComboKey))) {
                m_impl->droppedLinkLeft = m_impl->dragOut;
                ImGui::OpenPopup("DroppedLinkPopUp");
            }
        }

        // Links drag-out
        if (m_impl->dragOut->getType() == PinType_Output)
            detail::smart_bezier(
                m_impl->dragOut->pinPoint(),
                ImGui::GetMousePos(),
                m_impl->dragOut->getStyle().color,
                m_impl->dragOut->getStyle().extra.link_dragged_thickness);
        else
            detail::smart_bezier(
                ImGui::GetMousePos(),
                m_impl->dragOut->pinPoint(),
                m_impl->dragOut->getStyle().color,
                m_impl->dragOut->getStyle().extra.link_dragged_thickness);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            m_impl->dragOut = nullptr;
    }

    if (this->mouse_on_free_space() && !m_impl->box.active && !m_impl->draggingNode && !m_impl->dragOut && getSingleUseClick(getGrid().config().selection_box_button) && ImGui::IsWindowHovered()) {
        consumeSingleUseClick(getGrid().config().selection_box_button);
        m_impl->box.active = true;
        m_impl->box.selectionStart = ImGui::GetMousePos();

        // Deselect all nodes if not holding down CTRL
        if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
            for (auto& node : m_impl->nodes) {
                node.second->setSelected(false);
            }
        }
    }

    if (m_impl->box.active) {
        ImRect boxClip { m_impl->box.selectionStart, m_impl->box.selectionStart };
        boxClip.Add(ImGui::GetMousePos());

        draw_list->AddRectFilled(
            boxClip.GetBL(), boxClip.GetTR(), m_impl->box.selectionColor);
        draw_list->AddRect(
            boxClip.GetBL(),
            boxClip.GetTR(),
            m_impl->box.borderColor,
            0.0F,
            0,
            1.5F);

        bool shouldMarkSelected = false;

        if (ImGui::IsMouseReleased(
                getGrid().config().selection_box_button)) {
            m_impl->box.active = false;
            shouldMarkSelected = true;
        } else if (
            getGrid().config().selection_box_cancel_button != getGrid().config().selection_box_button && ImGui::IsMouseClicked(getGrid().config().selection_box_cancel_button)) {
            // cancel selection
            m_impl->box.active = false;
        }

        for (auto& elem : m_impl->nodes) {
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
    if (m_impl->rightClickPopUp && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
        m_impl->hoveredNodeAux = m_impl->hoveredNode;
        ImGui::OpenPopup("RightClickPopUp");
    }
    if (ImGui::BeginPopup("RightClickPopUp")) {
        m_impl->rightClickPopUp(m_impl->hoveredNodeAux);
        ImGui::EndPopup();
    }

    // Dropped Link PopUp
    if (ImGui::BeginPopup("DroppedLinkPopUp")) {
        m_impl->droppedLinkPopUp(m_impl->droppedLinkLeft);
        ImGui::EndPopup();
    }

    if (m_impl->context.hovered() && this->getSingleUseClick(m_impl->context.config().scroll_button)) {
        this->consumeSingleUseClick(m_impl->context.config().scroll_button);
        m_impl->draggingCanvas = true;
    }

    if (m_impl->draggingCanvas) {
        if (!ImGui::IsMouseDown(m_impl->context.config().scroll_button)) {
            m_impl->draggingCanvas = false;
        }
    }

    if (m_impl->draggingCanvas && ImGui::IsMouseDragging(m_impl->context.config().scroll_button)) {
        m_impl->context.updateScroll();
    }

    m_impl->context.end();

    if (ImGui::BeginDragDropTarget()) {
        if (m_impl->dragdropTarget) {
            m_impl->dragdropTarget(screen2grid(ImGui::GetMousePos()));
        }

        ImGui::EndDragDropTarget();
    }
}

} // namespace ImFlow

namespace ImFlow::detail {

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

void DynamicPin::update(bool* out_hovering)
{
    m_ttl = m_ttl > 0 ? m_ttl - 1 : m_ttl;
    this->pin().update(out_hovering);
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

} // namespace ImFlow::detail
