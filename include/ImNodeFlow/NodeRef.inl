#include "ImNodeFlow/PinInfo.h"
#include "ImNodeFlow/PinRef.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/detail/in_pin.h"
#include "ImNodeFlow/detail/out_pin.h"

#include "../../src/node.h"
#include "../../src/pin_base.h"

namespace ImFlow {

template <bool TIsConst>
template <typename T>
InPinRef<T, TIsConst>
NodeRefImpl<TIsConst>::addIN(InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    m_node.get().ins.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Input,
        BasePinInfo {
            .name = std::move(pin_info.name),
            .allow_same_node_connections = pin_info.allow_same_node_connections,
            .style = std::move(pin_info.style),
            .strat = pin_info.strat,
            .renderer = std::move(pin_info.renderer),
        },
        std::make_unique<detail::in_pin_t<T>>(pin_info)));

    return PinRef<TIsConst> { *m_node.get().ins.back() }.template toIn<T>();
}

template <bool TIsConst>
template <typename T>
const T&
NodeRefImpl<TIsConst>::showIN(InPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicIns) {
        if (PinRef<TIsConst> { dyn_pin.pin() }.getName() == pin_info.common.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            return PinRef<TIsConst> { dyn_pin.pin() }.template toIn<T>().value();
        }
    }

    m_node.get().dynamicIns.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Input,
        BasePinInfo {
            .name = std::move(pin_info.name),
            .allow_same_node_connections = pin_info.allow_same_node_connections,
            .style = std::move(pin_info.style),
            .strat = pin_info.strat,
            .renderer = std::move(pin_info.renderer),
        },
        std::make_unique<detail::in_pin_t<T>>(pin_info)));
    return PinRef<TIsConst> { m_node.get().dynamicIns.back().pin() }.template toIn<T>().value();
}

template <bool TIsConst>
template <typename T>
OutPinRef<T, TIsConst>
NodeRefImpl<TIsConst>::addOUT(OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    m_node.get().outs.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Output,
        BasePinInfo {
            .name = std::move(pin_info.name),
            .allow_same_node_connections = pin_info.allow_same_node_connections,
            .style = std::move(pin_info.style),
            .strat = pin_info.strat,
            .renderer = std::move(pin_info.renderer),
        },
        std::make_unique<detail::out_pin_t<T>>(pin_info)));
    return OutPinRef<T, TIsConst> {
        PinRef<TIsConst> { *m_node.get().outs.back() }.template toOut<T>()
    };
}

template <bool TIsConst>
template <typename T>
void NodeRefImpl<TIsConst>::showOUT(OutPinInfo<T> pin_info)
    requires(!TIsConst)
{
    for (auto& dyn_pin : m_node.get().dynamicOuts) {
        if (PinRef<TIsConst> { dyn_pin.pin() }.getName() == pin_info.name) {
            dyn_pin.refresh();
            // TODO: Update pin with new styles from pin_info
            PinRef<TIsConst> { dyn_pin.pin() }.template toOut<T>().behaviour(
                std::move(pin_info.behaviour));
            return;
        }
    }

    m_node.get().dynamicOuts.emplace_back(std::make_unique<detail::pin_base_t>(
        m_node.get().ctx,
        *this,
        PinType::Output,
        BasePinInfo {
            .name = std::move(pin_info.name),
            .allow_same_node_connections = pin_info.allow_same_node_connections,
            .style = std::move(pin_info.style),
            .strat = pin_info.strat,
            .renderer = std::move(pin_info.renderer),
        },
        std::make_unique<detail::out_pin_t<T>>(pin_info)));
}

template <bool TIsConst>
template <typename T>
std::optional<InPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::inPin(const PinUID& uid)
{
    auto pin = this->inPin(uid);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    return pin->template toIn<T>();
}

template <bool TIsConst>
template <typename T>
std::optional<InPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::inPin(const char* name)
{
    auto pin = this->inPin(name);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    return pin->template toIn<T>();
}

template <bool TIsConst>
template <typename T>
std::optional<OutPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::outPin(const PinUID& uid)
{
    auto pin = this->outPin(uid);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    return pin->template toOut<T>();
}

template <bool TIsConst>
template <typename T>
std::optional<OutPinRef<T, TIsConst>>
NodeRefImpl<TIsConst>::outPin(const char* name)
{
    auto pin = this->outPin(name);
    if (!pin.has_value()) {
        return std::nullopt;
    }

    return pin->template toOut<T>();
}

} // namespace ImFlow
