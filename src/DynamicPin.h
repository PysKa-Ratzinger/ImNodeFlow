#ifndef IMNODEFLOW_DYNAMICPIN_H
#define IMNODEFLOW_DYNAMICPIN_H
#pragma once

#include "ImNodeFlow/detail/fwd.h"

#include <cstdint>
#include <memory>

namespace ImFlow::detail {

/**
 * @brief A dynamic pin is a pin with a temporary lifetime, and will be marked
 * for deletion if not refreshed before every update
 */
class DynamicPin {
public:
    explicit DynamicPin(std::unique_ptr<IPin> pin);

    void refresh();
    void update(bool* out_hovering);
    IPin& pin();
    const IPin& pin() const;
    [[nodiscard]] bool isStale() const;

    std::unique_ptr<IPin> m_pin;
    uint32_t m_ttl = 1;
};

} // namespace ImFlow::detail

#endif // IMNODEFLOW_DYNAMICPIN_H
