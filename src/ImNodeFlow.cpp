#include "ImNodeFlow.h"

#include "./Context.impl"
#include "./INodeBehaviour.impl"
#include "./Link.impl"
#include "./uid_helpers.impl"

#include "./ImNodeFlow_internals.h"

#include "ImNodeFlow/IContainedContext.h"
#include "ImNodeFlow/NodeUID.h"

#include "imgui.h"

#include <cstdint>
#include <functional>
#include <imgui_internal.h>
#include <vector>

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace ImFlow {

// -----------------------------------------------------------------------------------------------------------------
// PIN

template <bool TIsConst>
void PinRef<TIsConst>::update(bool* out_hovering)
    requires(!TIsConst)
{
    auto& base = m_pin.get().toBase();

    if (!base.renderer) {
        ImGui::SetCursorPos(base.pos);
        ImGui::Text("%s", base.name.c_str());
    } else {
        ImGui::BeginGroup();
        base.renderer(*this);
        ImGui::EndGroup();
    }

    base.size = ImGui::GetItemRectSize();

    bool is_hovering = false;

    if (!base.renderer) {
        drawDecoration();
        drawSocket(&is_hovering);
    }

    // Calculate the combined hitbox if activated
    bool item_hovered = ImGui::IsItemHovered();
    bool socket_hovered = false;

    if (base.socketHitboxEnabled) {
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

template <bool TIsConst>
void PinRef<TIsConst>::drawSocket(bool* out_hovering_socket)
{
    auto& base = m_pin.get().toBase();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto [tl, br] = getSocketHitBounds();

    // Determine if we are in hover (checked now in update())
    bool socketHovered = ImGui::IsMouseHoveringRect(tl, br);

    if (isConnected())
        draw_list->AddCircleFilled(
            pinPoint(),
            base.style.socket_connected_radius,
            base.style.color,
            base.style.socket_shape);
    else {
        if (socketHovered) {
            draw_list->AddCircle(
                pinPoint(),
                base.style.socket_hovered_radius,
                base.style.color,
                base.style.socket_shape,
                base.style.socket_thickness);
        } else {
            draw_list->AddCircle(
                pinPoint(),
                base.style.socket_radius,
                base.style.color,
                base.style.socket_shape,
                base.style.socket_thickness);
        }
    }

    if (out_hovering_socket != nullptr) {
        *out_hovering_socket = socketHovered;
    }
}

template <bool TIsConst>
inline void
PinRef<TIsConst>::drawDecoration()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Check hover on text OR socket
    bool itemHovered = ImGui::IsItemHovered();
    bool socketHovered = false;

    auto& base = m_pin.get().toBase();

    if (base.socketHitboxEnabled) {
        auto [socket_tl, socket_br] = getSocketHitBounds();
        socketHovered = ImGui::IsMouseHoveringRect(socket_tl, socket_br);
    }

    if (itemHovered || socketHovered) {
        draw_list->AddRectFilled(
            base.pos - base.style.extra.padding,
            base.pos + base.size + base.style.extra.padding,
            base.style.extra.bg_hover_color,
            base.style.extra.bg_radius);
    } else {
        draw_list->AddRectFilled(
            base.pos - base.style.extra.padding,
            base.pos + base.size + base.style.extra.padding,
            base.style.extra.bg_color,
            base.style.extra.bg_radius);
    }

    draw_list->AddRect(
        base.pos - base.style.extra.padding,
        base.pos + base.size + base.style.extra.padding,
        base.style.extra.border_color,
        base.style.extra.bg_radius,
        0,
        base.style.extra.border_thickness);
}

template <bool TIsConst>
PinRef<TIsConst>&
PinRef<TIsConst>::renderer(std::function<void(PinRef<> p)> r)
    requires(!TIsConst)
{
    auto& base = m_pin.get().toBase();
    base.renderer = std::move(r);
    return *this;
}

template <bool TIsConst>
void PinRef<TIsConst>::linkTo(PinRef<TIsConst> other)
    requires(!TIsConst)
{
    auto& base = m_pin.get().toBase();
    auto& other_base = other.m_pin.get().toBase();

    if (&base.ctx.get() != &other_base.ctx.get()) {
        // Cannot link pins across different contexts
        return;
    }

    auto& ctx = base.ctx.get();
    ctx.createLink(*this, other);
}

template <bool TIsConst>
void PinRef<TIsConst>::deleteAllLinks()
    requires(!TIsConst)
{
    auto& base = m_pin.get().toBase();
    auto links = std::move(base.links);
}

template <bool TIsConst>
void PinRef<TIsConst>::addLink(detail::Link* link)
    requires(!TIsConst)
{
    auto& base = m_pin.get().toBase();

    auto iter = std::find(base.links.begin(), base.links.end(), link);

    if (iter != base.links.end()) {
        return;
    }

    if (base.strat == PinConnStrat::AllowOnlySingleConnection) {
        this->deleteAllLinks();
    }

    base.links.emplace_back(link);
}

template <bool TIsConst>
void PinRef<TIsConst>::removeLink(detail::Link* link)
    requires(!TIsConst)
{
    auto& base = m_pin.get().toBase();

    auto iter = std::find_if(
        base.links.begin(),
        base.links.end(),
        [link](const auto& internal_link) -> bool {
            return internal_link == link;
        });

    if (iter != base.links.end()) {
        base.links.erase(iter);
    }
}

template <bool TIsConst>
bool PinRef<TIsConst>::isConnected() const
{
    auto& base = m_pin.get().toBase();
    return !base.links.empty();
}

template <bool TIsConst>
PinUID PinRef<TIsConst>::getUid() const
{
    return m_pin.get().toBase().uid;
}

template <bool TIsConst>
const std::string& PinRef<TIsConst>::getName() const
{
    return m_pin.get().toBase().name;
}

template <bool TIsConst>
const ImVec2& PinRef<TIsConst>::getPos() const
{
    return m_pin.get().toBase().pos;
}

template <bool TIsConst>
const ImVec2& PinRef<TIsConst>::getSize() const
{
    return m_pin.get().toBase().size;
}

template <bool TIsConst>
NodeRefImpl<TIsConst> PinRef<TIsConst>::getParent() const
{
    return NodeRefImpl<TIsConst>(*m_pin.get().toBase().parent);
}

template <bool TIsConst>
PinType PinRef<TIsConst>::getType() const
{
    return m_pin.get().toBase().type;
}

template <bool TIsConst>
uint64_t PinRef<TIsConst>::getTypeID() const
{
    return m_pin.get().getTypeID();
}

template <bool TIsConst>
const PinStyle& PinRef<TIsConst>::getStyle() const
{
    return m_pin.get().toBase().style;
}

template <bool TIsConst>
void PinRef<TIsConst>::setStyle(PinStyle style)
    requires(!TIsConst)
{
    m_pin.get().toBase().style = std::move(style);
}

template <bool TIsConst>
void PinRef<TIsConst>::allowSameNodeConnections(bool state)
    requires(!TIsConst)
{
    m_pin.get().toBase().allowSelfConnection = state;
}

template <bool TIsConst>
bool PinRef<TIsConst>::allowsSameNodeConnections() const
{
    return m_pin.get().toBase().allowSelfConnection;
}

template <bool TIsConst>
ImVec2 PinRef<TIsConst>::pinPoint() const
{
    auto& base = m_pin.get().toBase();

    if (base.type == PinType::Input) {
        return base.pos + ImVec2(-base.style.extra.socket_padding, base.size.y / 2);
    }
    return base.pos + ImVec2(base.size.x + base.style.extra.socket_padding, base.size.y / 2);
}

template <bool TIsConst>
float PinRef<TIsConst>::calcWidth() const
{
    return ImGui::CalcTextSize(m_pin.get().toBase().name.c_str()).x;
}

template <bool TIsConst>
void PinRef<TIsConst>::setPos(ImVec2 pos)
    requires(!TIsConst)
{
    m_pin.get().toBase().pos = pos;
}

template <bool TIsConst>
inline std::pair<ImVec2, ImVec2>
PinRef<TIsConst>::getSocketHitBounds(float expand_radius)
{
    auto& base = m_pin.get().toBase();

    if (expand_radius < 0.0f) {
        expand_radius = base.style.socket_hovered_radius;
    }

    ImVec2 center = pinPoint();
    ImVec2 tl = center - ImVec2(expand_radius, expand_radius);
    ImVec2 br = center + ImVec2(expand_radius, expand_radius);

    return { tl, br };
}

template <bool TIsConst>
void PinRef<TIsConst>::setSocketHitboxEnabled(bool enabled)
    requires(!TIsConst)
{
    m_pin.get().toBase().socketHitboxEnabled = enabled;
}

template <bool TIsConst>
void PinRef<TIsConst>::calculate()
    requires(!TIsConst)
{
    m_pin.get().calculate();
}

template <bool TIsConst>
void PinRef<TIsConst>::prime()
    requires(!TIsConst)
{
    m_pin.get().prime();
}

template class PinRef<false>;
template class PinRef<true>;

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

template <bool TIsConst>
NodeRefImpl<TIsConst>::NodeRefImpl(underlying_type& internal_node)
    : m_node(internal_node)
{
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::destroy()
    requires(!TIsConst)
{
    if (m_node.get().behaviour) {
        m_node.get().behaviour.reset();
    }
    m_node.get().destroyed = true;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::toDestroy() const
{
    return m_node.get().destroyed;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::isHovered() const
{
    ImVec2 paddingTL = { m_node.get().style.padding.x, m_node.get().style.padding.y };
    ImVec2 paddingBR = { m_node.get().style.padding.z, m_node.get().style.padding.w };
    return ImGui::IsMouseHoveringRect(
        m_node.get().ctx.get().grid2screen(m_node.get().pos - paddingTL),
        m_node.get().ctx.get().grid2screen(m_node.get().pos + m_node.get().size + paddingBR));
}

template <bool TIsConst>
NodeUID NodeRefImpl<TIsConst>::getUID() const
{
    return m_node.get().uid;
}

template <bool TIsConst>
const std::string& NodeRefImpl<TIsConst>::getName() const
{
    return m_node.get().title;
}

template <bool TIsConst>
const ImVec2& NodeRefImpl<TIsConst>::getSize() const
{
    return m_node.get().size;
}

template <bool TIsConst>
const ImVec2& NodeRefImpl<TIsConst>::getFullSize() const
{
    return m_node.get().fullSize;
}

template <bool TIsConst>
const ImVec2& NodeRefImpl<TIsConst>::getPos() const
{
    return m_node.get().pos;
}

template <bool TIsConst>
ImVec2 NodeRefImpl<TIsConst>::getTLPos() const
{
    return m_node.get().pos - ImVec2 { m_node.get().style.padding.x, m_node.get().style.padding.y };
}

template <bool TIsConst>
const NodeStyle& NodeRefImpl<TIsConst>::getStyle() const
{
    return m_node.get().style;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::isSelected() const
{
    return m_node.get().selected || m_node.get().provisionallySelected;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::isDragged() const
{
    return m_node.get().dragged;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::isIndestructable() const
{
    return m_node.get().indestructable;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setUID(NodeUID uid)
    requires(!TIsConst)
{
    m_node.get().uid = uid;
    return *this;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setTitle(const std::string& title)
    requires(!TIsConst)
{
    m_node.get().title = title;
    return *this;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setPos(const ImVec2& pos)
    requires(!TIsConst)
{
    m_node.get().pos = pos;
    m_node.get().posTarget = pos;
    return *this;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setStyle(NodeStyle style)
    requires(!TIsConst)
{
    m_node.get().style = std::move(style);
    return *this;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setIndestructable(bool indestruct)
    requires(!TIsConst)
{
    m_node.get().indestructable = indestruct;
    return *this;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setSelected(bool state)
    requires(!TIsConst)
{
    m_node.get().selectedNext = state;
    return *this;
}

template <bool TIsConst>
NodeRefImpl<TIsConst>& NodeRefImpl<TIsConst>::setProvisionallySelected(bool state)
    requires(!TIsConst)
{
    m_node.get().provisionallySelectedNext = state;
    return *this;
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::updatePublicStatus()
    requires(!TIsConst)
{
    m_node.get().selected = m_node.get().selectedNext;
    m_node.get().provisionallySelected = m_node.get().provisionallySelectedNext;
    m_node.get().provisionallySelectedNext = false;
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropIN(const PinUID& uid)
    requires(!TIsConst)
{
    auto iter = std::find_if(
        m_node.get().ins.begin(), m_node.get().ins.end(), [&](auto const& pin) -> bool {
            return pin->toBase().uid == uid;
        });

    if (iter != m_node.get().ins.end()) {
        m_node.get().ins.erase(iter);
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropIN(const char* name)
    requires(!TIsConst)
{
    m_node.get().ins.erase(
        std::remove_if(
            m_node.get().ins.begin(),
            m_node.get().ins.end(),
            [&](auto const& pin) -> bool {
                return pin->toBase().name == name;
            }),
        m_node.get().ins.end());
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropOUT(const PinUID& uid)
    requires(!TIsConst)
{
    auto iter = std::find_if(
        m_node.get().outs.begin(),
        m_node.get().outs.end(),
        [&uid](auto const& pin) -> bool {
            return pin->toBase().uid == uid;
        });

    if (iter != m_node.get().outs.end()) {
        m_node.get().outs.erase(iter);
    }
}

template <bool TIsConst>
inline void
NodeRefImpl<TIsConst>::dropOUT(const char* name)
    requires(!TIsConst)
{
    m_node.get().outs.erase(
        std::remove_if(
            m_node.get().outs.begin(),
            m_node.get().outs.end(),
            [&name](auto const& pin) -> bool {
                return pin->toBase().name == name;
            }),
        m_node.get().outs.end());
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::inPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_node.get().ins.begin(),
        m_node.get().ins.end(),
        [&uid](const auto& p) {
            return p->toBase().uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().ins.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::inPin(const char* name)
{
    auto it = std::find_if(
        m_node.get().ins.begin(),
        m_node.get().ins.end(),
        [&name](const auto& p) {
            return p->toBase().name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().ins.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::outPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_node.get().outs.begin(),
        m_node.get().outs.end(),
        [&uid](const auto& p) {
            return p->toBase().uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().outs.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::outPin(const char* name)
{
    auto it = std::find_if(
        m_node.get().outs.begin(),
        m_node.get().outs.end(),
        [&name](const auto& p) {
            return p->toBase().name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().outs.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::calculate()
    requires(!TIsConst)
{
    for (auto& pin : m_node.get().outs) {
        pin->calculate();
    }
    for (auto& pin : m_node.get().dynamicOuts) {
        pin.pin().calculate();
    }
    for (auto& pin : m_node.get().ins) {
        pin->calculate();
    }
    for (auto& pin : m_node.get().dynamicIns) {
        pin.pin().calculate();
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::prime()
    requires(!TIsConst)
{
    for (auto& pin : m_node.get().outs) {
        pin->prime();
    }
    for (auto& pin : m_node.get().dynamicOuts) {
        pin.pin().prime();
    }
    for (auto& pin : m_node.get().ins) {
        pin->prime();
    }
    for (auto& pin : m_node.get().dynamicIns) {
        pin.pin().prime();
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::update()
    requires(!TIsConst)
{
    auto& ctx = m_node.get().ctx.get();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::PushID(&m_node.get());
    ImVec2 offset = ctx.grid2screen({ 0.f, 0.f });
    ImVec2 paddingTL = { m_node.get().style.padding.x, m_node.get().style.padding.y };
    ImVec2 paddingBR = { m_node.get().style.padding.z, m_node.get().style.padding.w };

    draw_list->ChannelsSetCurrent(1); // Foreground
    ImGui::SetCursorScreenPos(offset + m_node.get().pos);

    ImGui::BeginGroup();

    // Header
    ImGui::BeginGroup();
    ImGui::TextColored(m_node.get().style.header_title_color, "%s", m_node.get().title.c_str());
    ImGui::Spacing();
    ImGui::EndGroup();
    float headerH = ImGui::GetItemRectSize().y;
    float titleW = ImGui::GetItemRectSize().x;

    detail::IPin* hovered_pin = nullptr;

    // Inputs
    if (!m_node.get().ins.empty() || !m_node.get().dynamicIns.empty()) {
        ImGui::BeginGroup();
        for (auto& p : m_node.get().ins) {
            PinRef pin_ref { *p };

            pin_ref.setPos(ImGui::GetCursorPos());
            bool is_hovering = false;
            pin_ref.update(&is_hovering);
            if (is_hovering && hovered_pin == nullptr) {
                hovered_pin = &*p;
            }
        }
        for (auto& dyn_pin : m_node.get().dynamicIns) {
            PinRef pin_ref { dyn_pin.pin() };

            pin_ref.setPos(ImGui::GetCursorPos());
            bool is_hovering = false;
            pin_ref.update(&is_hovering);
            if (is_hovering && hovered_pin == nullptr) {
                hovered_pin = &dyn_pin.pin();
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();
    }

    // Content
    ImGui::BeginGroup();

    if (m_node.get().behaviour) {
        m_node.get().behaviour->draw(*this);
    } else {
        ImGui::TextUnformatted("<no renderer provided>");
    }

    // Dynamic inputs stale check can only be done after the call to
    // behaviour->draw() and before any call to DynamicPin::update()
    m_node.get().dynamicIns.erase(
        std::remove_if(
            m_node.get().dynamicIns.begin(),
            m_node.get().dynamicIns.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        m_node.get().dynamicIns.end());
    m_node.get().dynamicOuts.erase(
        std::remove_if(
            m_node.get().dynamicOuts.begin(),
            m_node.get().dynamicOuts.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        m_node.get().dynamicOuts.end());

    ImGui::Dummy(ImVec2(0.f, 0.f));
    ImGui::EndGroup();
    ImGui::SameLine();

    // Outputs
    float maxW = 0.0f;
    for (auto& p : m_node.get().outs) {
        float w = PinRef { *p }.calcWidth();
        if (w > maxW)
            maxW = w;
    }
    for (auto& dyn_pin : m_node.get().dynamicOuts) {
        float w = PinRef { dyn_pin.pin() }.calcWidth();
        if (w > maxW)
            maxW = w;
    }
    ImGui::BeginGroup();
    for (auto& p : m_node.get().outs) {
        PinRef pin { *p };

        // FIXME: This looks horrible
        if ((m_node.get().pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            pin.setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - pin.calcWidth(), 0.f));
        else
            pin.setPos(ImVec2(
                (m_node.get().pos + ImVec2(titleW - pin.calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        bool is_hovering = false;
        pin.update(&is_hovering);
        if (is_hovering && hovered_pin == nullptr) {
            hovered_pin = &*p;
        }
    }
    for (auto& dyn_pin : m_node.get().dynamicOuts) {
        PinRef pin { dyn_pin.pin() };

        // FIXME: This looks horrible
        if ((m_node.get().pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            pin.setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - pin.calcWidth(), 0.f));
        else
            pin.setPos(ImVec2(
                (m_node.get().pos + ImVec2(titleW - pin.calcWidth(), 0) + ctx.getGrid().scroll())
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
    m_node.get().size = ImGui::GetItemRectSize();
    ImVec2 headerSize = ImVec2(m_node.get().size.x + paddingBR.x, headerH);

    // Background
    draw_list->ChannelsSetCurrent(0);
    draw_list->AddRectFilled(
        offset + m_node.get().pos - paddingTL,
        offset + m_node.get().pos + m_node.get().size + paddingBR,
        m_node.get().style.bg,
        m_node.get().style.radius);
    draw_list->AddRectFilled(
        offset + m_node.get().pos - paddingTL,
        offset + m_node.get().pos + headerSize,
        m_node.get().style.header_bg,
        m_node.get().style.radius,
        ImDrawFlags_RoundCornersTop);
    m_node.get().fullSize = m_node.get().size + paddingTL + paddingBR;
    ImU32 col = m_node.get().style.border_color;
    float thickness = m_node.get().style.border_thickness;
    ImVec2 ptl = paddingTL;
    ImVec2 pbr = paddingBR;
    if (this->isSelected()) {
        col = m_node.get().style.border_selected_color;
        thickness = m_node.get().style.border_selected_thickness;
    }
    if (thickness < 0.f) {
        ptl.x -= thickness / 2;
        ptl.y -= thickness / 2;
        pbr.x -= thickness / 2;
        pbr.y -= thickness / 2;
        thickness *= -1.f;
    }
    draw_list->AddRect(
        offset + m_node.get().pos - ptl,
        offset + m_node.get().pos + m_node.get().size + pbr,
        col,
        m_node.get().style.radius,
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
            offset + m_node.get().pos - paddingTL, offset + m_node.get().pos + headerSize);

        ctx.hoveredNode(&m_node.get());
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            setSelected(true);
            if (onHeader) {
                m_node.get().dragged = true;
                ctx.draggingNode(true);
            }
        }
    }

    if (m_node.get().dragged || (m_node.get().selected && ctx.isNodeDragged())) {
        float step = ctx.getStyle().grid_size / ctx.getStyle().grid_subdivisions;
        m_node.get().posTarget += ctx.getScreenSpaceDelta();
        // "Slam" The position
        m_node.get().pos.x = round(m_node.get().posTarget.x / step) * step;
        m_node.get().pos.y = round(m_node.get().posTarget.y / step) * step;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_node.get().dragged = false;
            ctx.draggingNode(false);
            m_node.get().posTarget = m_node.get().pos;
        }
    }
    ImGui::PopID();
}

template class NodeRefImpl<false>;
template class NodeRefImpl<true>;

IContainedContext::~IContainedContext() = default;

} // namespace ImFlow

namespace ImFlow::detail {

// -----------------------------------------------------------------------------------------------------------------
// DYNAMIC PIN

DynamicPin::DynamicPin(std::unique_ptr<IPin> pin)
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
    PinRef<> { this->pin() }.update(out_hovering);
}

IPin& DynamicPin::pin()
{
    return *m_pin;
}

const IPin&
DynamicPin::pin() const
{
    return *m_pin;
}

bool DynamicPin::isStale() const
{
    return m_ttl == 0;
}

// -----------------------------------------------------------------------------------------------------------------
// PIN CONNECTION HOLDER

PinConnectionHolder::PinConnectionHolder(Link* link) noexcept
    : m_link_ptr(link)
{
}

PinConnectionHolder::~PinConnectionHolder() noexcept
{
    this->destroy();
}

PinConnectionHolder::PinConnectionHolder(PinConnectionHolder&& other) noexcept
    : m_link_ptr(other.m_link_ptr)
{
    other.m_link_ptr = nullptr;
}

PinConnectionHolder& PinConnectionHolder::operator=(PinConnectionHolder&& other) noexcept
{
    this->destroy();
    this->m_link_ptr = other.m_link_ptr;
    other.m_link_ptr = nullptr;
    return *this;
}

void PinConnectionHolder::destroy()
{
    if (m_link_ptr != nullptr) {
        auto* link = m_link_ptr;
        m_link_ptr = nullptr;
        link->destroy();
    }
}

} // namespace ImFlow::detail
