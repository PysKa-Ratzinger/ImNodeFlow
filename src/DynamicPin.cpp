#include "./DynamicPin.h"

#include "ImNodeFlow/PinRef.h"

#include <memory>

namespace ImFlow::detail {

DynamicPin::DynamicPin(std::unique_ptr<detail::pin_base_t> pin)
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
    PinRef<> { this->pin() }.update();
}

detail::pin_base_t& DynamicPin::pin()
{
    return *m_pin;
}

const detail::pin_base_t&
DynamicPin::pin() const
{
    return *m_pin;
}

bool DynamicPin::isStale() const
{
    return m_ttl == 0;
}

} // namespace ImFlow::detail
