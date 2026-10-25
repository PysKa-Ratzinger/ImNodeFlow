#include "ImNodeFlow.h"

#include "./Context.impl"
#include "./DynamicPin.impl"
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
