#ifndef IMNODEFLOW_PINCONNECTIONHOLDER_H
#define IMNODEFLOW_PINCONNECTIONHOLDER_H
#pragma once

#include "./Link.h"

namespace ImFlow::detail {

/**
 * @brief Representation of an established connection from the pin's
 * perspective.
 */
class PinConnectionHolder {
public:
    PinConnectionHolder() noexcept = default;

    /// Connection was established
    explicit PinConnectionHolder(Link* link) noexcept;
    /// Connection was destroyed
    ~PinConnectionHolder() noexcept;

    // delete copy-ctor (connections are unique and cannot be copied)
    PinConnectionHolder(const PinConnectionHolder&) = delete;
    // delete copy-operator (connections are unique and cannot be copied)
    PinConnectionHolder& operator=(const PinConnectionHolder&) = delete;

    /// Connection relocated in memory for some reason
    PinConnectionHolder(PinConnectionHolder&&) noexcept;
    /// Connection relocated in memory for some reason
    PinConnectionHolder& operator=(PinConnectionHolder&&) noexcept;

    Link* link() { return m_link_ptr; }
    Link const* link() const { return m_link_ptr; }

    [[nodiscard]] constexpr bool operator==(Link* link) const noexcept
    {
        return m_link_ptr == link;
    }

    void destroy();

private:
    Link* m_link_ptr = nullptr;
};

} // namespace ImFlow::detail

#endif // IMNODEFLOW_PINCONNECTIONHOLDER_H
