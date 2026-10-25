#include "ImNodeFlow.h"

#include "./Context.impl"
#include "./INodeBehaviour.impl"
#include "./Link.impl"
#include "./NodeRef.impl"
#include "./PinRef.impl"
#include "./uid_helpers.impl"

#include "./ImNodeFlow_internals.h"

#include "ImNodeFlow/IContainedContext.h"

#include "imgui.h"

#include <imgui_internal.h>

#include <cassert>
#include <memory>
#include <utility>

namespace ImFlow {

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
