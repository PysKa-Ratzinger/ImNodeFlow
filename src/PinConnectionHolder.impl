#include "./PinConnectionHolder.h"

#include "./Link.h"

namespace ImFlow::detail {

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
