#include "ImNodeFlow/NodeRef.h"

#include "ImNodeFlow/Context.h"
#include "ImNodeFlow/NodeStyle.h"
#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/PinRef.h"
#include "ImNodeFlow/PinType.h"

#include "imgui.h"

#include "./node.h"
#include "./pin_base.h"

#include <string_view>
#include <utility>

namespace ImFlow {

template <bool TIsConst>
NodeRefImpl<TIsConst>::NodeRefImpl(underlying_type& internal_node)
    : m_node(internal_node)
{
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::destroy()
    requires(!TIsConst)
{
    auto& node = m_node.get();
    if (node.behaviour) {
        node.behaviour.reset();
    }
    node.destroyed = true;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::toDestroy() const
{
    return m_node.get().destroyed;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::isHovered() const
{
    auto& node = m_node.get();
    ImVec2 paddingTL = { node.style.padding.x, node.style.padding.y };
    ImVec2 paddingBR = { node.style.padding.z, node.style.padding.w };
    return ImGui::IsMouseHoveringRect(
        node.ctx.get().grid2screen(node.pos - paddingTL),
        node.ctx.get().grid2screen(node.pos + node.size + paddingBR));
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
    auto& node = m_node.get();
    return node.pos - ImVec2 { node.style.padding.x, node.style.padding.y };
}

template <bool TIsConst>
const NodeStyle& NodeRefImpl<TIsConst>::getStyle() const
{
    return m_node.get().style;
}

template <bool TIsConst>
bool NodeRefImpl<TIsConst>::isSelected() const
{
    auto& node = m_node.get();
    return node.selected || node.provisionallySelected;
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
    auto& node = m_node.get();
    node.pos = pos;
    node.posTarget = pos;
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
    auto& node = m_node.get();
    node.selected = node.selectedNext;
    node.provisionallySelected = node.provisionallySelectedNext;
    node.provisionallySelectedNext = false;
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropIN(const PinUID& uid)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    auto iter = std::find_if(
        node.ins.begin(),
        node.ins.end(),
        [&](auto const& pin) -> bool {
            return pin->uid == uid;
        });

    if (iter != node.ins.end()) {
        node.ins.erase(iter);
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropIN(std::string_view name)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    node.ins.erase(
        std::remove_if(
            node.ins.begin(),
            node.ins.end(),
            [&](auto const& pin) -> bool {
                return pin->name == name;
            }),
        node.ins.end());
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropOUT(const PinUID& uid)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    auto iter = std::find_if(
        node.outs.begin(),
        node.outs.end(),
        [&uid](auto const& pin) -> bool {
            return pin->uid == uid;
        });

    if (iter != node.outs.end()) {
        node.outs.erase(iter);
    }
}

template <bool TIsConst>
inline void
NodeRefImpl<TIsConst>::dropOUT(std::string_view name)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    node.outs.erase(
        std::remove_if(
            node.outs.begin(),
            node.outs.end(),
            [&name](auto const& pin) -> bool {
                return pin->name == name;
            }),
        node.outs.end());
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::inPin(const PinUID& uid)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.ins.begin(),
        node.ins.end(),
        [&uid](const auto& p) {
            return p->uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.ins.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::inPin(std::string_view name)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.ins.begin(),
        node.ins.end(),
        [&name](const auto& p) {
            return p->name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.ins.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::tempInPin(const PinUID& uid)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.dynamicIns.begin(),
        node.dynamicIns.end(),
        [&uid](const auto& p) {
            return p.pin().uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.dynamicIns.end()) {
        if constexpr (!TIsConst) {
            it->refresh();
        }
        result.emplace(it->pin());
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::tempInPin(std::string_view name)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.dynamicIns.begin(),
        node.dynamicIns.end(),
        [&name](const auto& p) {
            return p.pin().name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.dynamicIns.end()) {
        if constexpr (!TIsConst) {
            it->refresh();
        }
        result.emplace(it->pin());
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::outPin(const PinUID& uid)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.outs.begin(),
        node.outs.end(),
        [&uid](const auto& p) {
            return p->uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.outs.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::outPin(std::string_view name)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.outs.begin(),
        node.outs.end(),
        [&name](const auto& p) {
            return p->name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.outs.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::tempOutPin(const PinUID& uid)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.dynamicOuts.begin(),
        node.dynamicOuts.end(),
        [&uid](const auto& p) {
            return p.pin().uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.dynamicOuts.end()) {
        if constexpr (!TIsConst) {
            it->refresh();
        }
        result.emplace(it->pin());
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::tempOutPin(std::string_view name)
{
    auto& node = m_node.get();
    auto it = std::find_if(
        node.dynamicOuts.begin(),
        node.dynamicOuts.end(),
        [&name](const auto& p) {
            return p.pin().name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != node.dynamicOuts.end()) {
        if constexpr (!TIsConst) {
            it->refresh();
        }
        result.emplace(it->pin());
    }
    return result;
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::calculate()
    requires(!TIsConst)
{
    auto& node = m_node.get();
    for (auto& pin : node.outs) {
        pin->underlying_pin->calculate(PinRef { *pin });
    }
    for (auto& pin : node.dynamicOuts) {
        pin.pin().underlying_pin->calculate(PinRef { pin.pin() });
    }
    for (auto& pin : node.ins) {
        pin->underlying_pin->calculate(PinRef { *pin });
    }
    for (auto& pin : node.dynamicIns) {
        pin.pin().underlying_pin->calculate(PinRef { pin.pin() });
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::prime()
    requires(!TIsConst)
{
    auto& node = m_node.get();
    for (auto& pin : node.outs) {
        pin->underlying_pin->prime();
    }
    for (auto& pin : node.dynamicOuts) {
        pin.pin().underlying_pin->prime();
    }
    for (auto& pin : node.ins) {
        pin->underlying_pin->prime();
    }
    for (auto& pin : node.dynamicIns) {
        pin.pin().underlying_pin->prime();
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::update()
    requires(!TIsConst)
{
    auto& node = m_node.get();
    auto& ctx = node.ctx.get();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::PushID(&node);
    ImVec2 offset = ctx.grid2screen({ 0.f, 0.f });
    ImVec2 paddingTL = { node.style.padding.x, node.style.padding.y };
    ImVec2 paddingBR = { node.style.padding.z, node.style.padding.w };

    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1); // Foreground
    ImGui::SetCursorScreenPos(offset + node.pos);

    ImGui::BeginGroup();

    // Header
    ImGui::BeginGroup();
    ImGui::TextColored(node.style.header_title_color, "%s", node.title.c_str());
    ImGui::Spacing();
    ImGui::EndGroup();
    float headerH = ImGui::GetItemRectSize().y;
    float titleW = ImGui::GetItemRectSize().x;

    detail::pin_base_t* hovered_pin = nullptr;

    // Inputs
    if (!node.ins.empty() || !node.dynamicIns.empty()) {
        ImGui::BeginGroup();
        for (auto& p : node.ins) {
            PinRef pin_ref { *p };

            pin_ref.setPos(ImGui::GetCursorPos());
            bool is_hovering = false;
            pin_ref.update(&is_hovering);
            if (is_hovering && hovered_pin == nullptr) {
                hovered_pin = &*p;
            }
        }
        for (auto& dyn_pin : node.dynamicIns) {
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

    if (node.behaviour) {
        node.behaviour->draw(*this);
    } else {
        ImGui::TextUnformatted("<no renderer provided>");
    }

    // Dynamic inputs stale check can only be done after the call to
    // behaviour->draw() and before any call to DynamicPin::update()
    node.dynamicIns.erase(
        std::remove_if(
            node.dynamicIns.begin(),
            node.dynamicIns.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        node.dynamicIns.end());
    node.dynamicOuts.erase(
        std::remove_if(
            node.dynamicOuts.begin(),
            node.dynamicOuts.end(),
            [](const auto& elem) -> bool { return elem.isStale(); }),
        node.dynamicOuts.end());

    ImGui::Dummy(ImVec2(0.f, 0.f));
    ImGui::EndGroup();
    ImGui::SameLine();

    // Outputs
    float maxW = 0.0f;
    for (auto& p : node.outs) {
        float w = PinRef { *p }.calcWidth();
        if (w > maxW)
            maxW = w;
    }
    for (auto& dyn_pin : node.dynamicOuts) {
        float w = PinRef { dyn_pin.pin() }.calcWidth();
        if (w > maxW)
            maxW = w;
    }
    ImGui::BeginGroup();
    for (auto& p : node.outs) {
        PinRef pin { *p };

        // FIXME: This looks horrible
        if ((node.pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            pin.setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - pin.calcWidth(), 0.f));
        else
            pin.setPos(ImVec2(
                (node.pos + ImVec2(titleW - pin.calcWidth(), 0) + ctx.getGrid().scroll())
                    .x,
                ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
        bool is_hovering = false;
        pin.update(&is_hovering);
        if (is_hovering && hovered_pin == nullptr) {
            hovered_pin = &*p;
        }
    }
    for (auto& dyn_pin : node.dynamicOuts) {
        PinRef pin { dyn_pin.pin() };

        // FIXME: This looks horrible
        if ((node.pos + ImVec2(titleW, 0) + ctx.getGrid().scroll()).x < ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
            pin.setPos(
                ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - pin.calcWidth(), 0.f));
        else
            pin.setPos(ImVec2(
                (node.pos + ImVec2(titleW - pin.calcWidth(), 0) + ctx.getGrid().scroll())
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
    node.size = ImGui::GetItemRectSize();
    ImVec2 headerSize = ImVec2(node.size.x + paddingBR.x, headerH);

    // Background
    draw_list->ChannelsSetCurrent(0);
    draw_list->AddRectFilled(
        offset + node.pos - paddingTL,
        offset + node.pos + node.size + paddingBR,
        node.style.bg,
        node.style.radius);
    draw_list->AddRectFilled(
        offset + node.pos - paddingTL,
        offset + node.pos + headerSize,
        node.style.header_bg,
        node.style.radius,
        ImDrawFlags_RoundCornersTop);
    node.fullSize = node.size + paddingTL + paddingBR;
    ImU32 col = node.style.border_color;
    float thickness = node.style.border_thickness;
    ImVec2 ptl = paddingTL;
    ImVec2 pbr = paddingBR;
    if (this->isSelected()) {
        col = node.style.border_selected_color;
        thickness = node.style.border_selected_thickness;
    }
    if (thickness < 0.f) {
        ptl.x -= thickness / 2;
        ptl.y -= thickness / 2;
        pbr.x -= thickness / 2;
        pbr.y -= thickness / 2;
        thickness *= -1.f;
    }
    draw_list->AddRect(
        offset + node.pos - ptl,
        offset + node.pos + node.size + pbr,
        col,
        node.style.radius,
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
            offset + node.pos - paddingTL, offset + node.pos + headerSize);

        ctx.hoveredNode(&node);
        if (ctx.getSingleUseClick(ImGuiMouseButton_Left)) {
            ctx.consumeSingleUseClick(ImGuiMouseButton_Left);
            setSelected(true);
            if (onHeader) {
                node.dragged = true;
                ctx.draggingNode(true);
            }
        }
    }

    if (node.dragged || (node.selected && ctx.isNodeDragged())) {
        float step = ctx.getStyle().grid_size / ctx.getStyle().grid_subdivisions;
        node.posTarget += ctx.getScreenSpaceDelta();
        // "Slam" The position
        node.pos.x = round(node.posTarget.x / step) * step;
        node.pos.y = round(node.posTarget.y / step) * step;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            node.dragged = false;
            ctx.draggingNode(false);
            node.posTarget = node.pos;
        }
    }

    draw_list->ChannelsMerge();
    ImGui::PopID();
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::addIN_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    node.ins.emplace_back(std::make_unique<detail::pin_base_t>(
        node.ctx,
        *this,
        PinType::Input,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { *node.ins.back() };
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::showIN_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    node.dynamicIns.emplace_back(std::make_unique<detail::pin_base_t>(
        node.ctx,
        *this,
        PinType::Input,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { node.dynamicIns.back().pin() };
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::addOUT_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    node.outs.emplace_back(std::make_unique<detail::pin_base_t>(
        node.ctx,
        *this,
        PinType::Output,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { *node.outs.back() };
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::showOUT_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    auto& node = m_node.get();
    node.dynamicOuts.emplace_back(std::make_unique<detail::pin_base_t>(
        node.ctx,
        *this,
        PinType::Output,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { node.dynamicOuts.back().pin() };
}

template class NodeRefImpl<false>;
template class NodeRefImpl<true>;

} // namespace ImFlow
