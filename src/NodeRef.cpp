#include "ImNodeFlow/NodeRef.h"

#include "ImNodeFlow/NodeStyle.h"
#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/PinRef.h"
#include "ImNodeFlow/PinType.h"

#include "imgui.h"

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
        m_node.get().ins.begin(),
        m_node.get().ins.end(),
        [&](auto const& pin) -> bool {
            return pin->uid == uid;
        });

    if (iter != m_node.get().ins.end()) {
        m_node.get().ins.erase(iter);
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::dropIN(std::string_view name)
    requires(!TIsConst)
{
    m_node.get().ins.erase(
        std::remove_if(
            m_node.get().ins.begin(),
            m_node.get().ins.end(),
            [&](auto const& pin) -> bool {
                return pin->name == name;
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
            return pin->uid == uid;
        });

    if (iter != m_node.get().outs.end()) {
        m_node.get().outs.erase(iter);
    }
}

template <bool TIsConst>
inline void
NodeRefImpl<TIsConst>::dropOUT(std::string_view name)
    requires(!TIsConst)
{
    m_node.get().outs.erase(
        std::remove_if(
            m_node.get().outs.begin(),
            m_node.get().outs.end(),
            [&name](auto const& pin) -> bool {
                return pin->name == name;
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
            return p->uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().ins.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::inPin(std::string_view name)
{
    auto it = std::find_if(
        m_node.get().ins.begin(),
        m_node.get().ins.end(),
        [&name](const auto& p) {
            return p->name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().ins.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::tempInPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_node.get().dynamicIns.begin(),
        m_node.get().dynamicIns.end(),
        [&uid](const auto& p) {
            return p.pin().uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().dynamicIns.end()) {
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
    auto it = std::find_if(
        m_node.get().dynamicIns.begin(),
        m_node.get().dynamicIns.end(),
        [&name](const auto& p) {
            return p.pin().name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().dynamicIns.end()) {
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
    auto it = std::find_if(
        m_node.get().outs.begin(),
        m_node.get().outs.end(),
        [&uid](const auto& p) {
            return p->uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().outs.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::outPin(std::string_view name)
{
    auto it = std::find_if(
        m_node.get().outs.begin(),
        m_node.get().outs.end(),
        [&name](const auto& p) {
            return p->name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().outs.end()) {
        result.emplace(**it);
    }
    return result;
}

template <bool TIsConst>
std::optional<PinRef<TIsConst>>
NodeRefImpl<TIsConst>::tempOutPin(const PinUID& uid)
{
    auto it = std::find_if(
        m_node.get().dynamicOuts.begin(),
        m_node.get().dynamicOuts.end(),
        [&uid](const auto& p) {
            return p.pin().uid == uid;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().dynamicOuts.end()) {
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
    auto it = std::find_if(
        m_node.get().dynamicOuts.begin(),
        m_node.get().dynamicOuts.end(),
        [&name](const auto& p) {
            return p.pin().name == name;
        });
    std::optional<PinRef<TIsConst>> result;
    if (it != m_node.get().dynamicOuts.end()) {
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
    for (auto& pin : m_node.get().outs) {
        pin->underlying_pin->calculate(PinRef { *pin });
    }
    for (auto& pin : m_node.get().dynamicOuts) {
        pin.pin().underlying_pin->calculate(PinRef { pin.pin() });
    }
    for (auto& pin : m_node.get().ins) {
        pin->underlying_pin->calculate(PinRef { *pin });
    }
    for (auto& pin : m_node.get().dynamicIns) {
        pin.pin().underlying_pin->calculate(PinRef { pin.pin() });
    }
}

template <bool TIsConst>
void NodeRefImpl<TIsConst>::prime()
    requires(!TIsConst)
{
    for (auto& pin : m_node.get().outs) {
        pin->underlying_pin->prime();
    }
    for (auto& pin : m_node.get().dynamicOuts) {
        pin.pin().underlying_pin->prime();
    }
    for (auto& pin : m_node.get().ins) {
        pin->underlying_pin->prime();
    }
    for (auto& pin : m_node.get().dynamicIns) {
        pin.pin().underlying_pin->prime();
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

    detail::pin_base_t* hovered_pin = nullptr;

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

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::addIN_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    m_node.get().ins.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Input,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { *m_node.get().ins.back() };
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::showIN_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    m_node.get().dynamicIns.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Input,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { m_node.get().dynamicIns.back().pin() };
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::addOUT_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    m_node.get().outs.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Output,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { *m_node.get().outs.back() };
}

template <bool TIsConst>
PinRef<TIsConst>
NodeRefImpl<TIsConst>::showOUT_impl(
    BasePinInfo info,
    std::unique_ptr<detail::IPin> pin_impl)
    requires(!TIsConst)
{
    m_node.get().dynamicOuts.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Output,
        std::move(info),
        std::move(pin_impl)));

    return PinRef<TIsConst> { m_node.get().dynamicOuts.back().pin() };
}

template class NodeRefImpl<false>;
template class NodeRefImpl<true>;

} // namespace ImFlow
