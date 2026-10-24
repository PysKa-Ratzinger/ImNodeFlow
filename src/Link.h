#ifndef IMNODEFLOW_SRC_LINK_H
#define IMNODEFLOW_SRC_LINK_H
#pragma once

#include "ImNodeFlow/fwd.h"

#include "./ImNodeFlow_internals.h"

namespace ImFlow::detail {

/**
 * @brief Link between two Pins of two different Nodes
 */
class Link {
public:
    /**
     * @brief Construct a link
     * @param out_pin Pointer to the output Pin of the Link
     * @param in_pin Pointer to the input Pin of the Link
     */
    explicit Link(IPin& out_pin, IPin& in_pin) noexcept;

    /**
     * @brief Destruction of a link
     * @details Deletes references of this links form connected pins
     */
    ~Link() noexcept;

    /**
     * @brief Looping function to update the Link
     * @param ctx The ImNodeFlow context containing this link
     * @details Draws the Link and updates Hovering and Selected status.
     */
    void update(ImNodeFlow& ctx);

    /**
     * @brief Get output pin of the link
     * @return Reference to the Pin
     */
    [[nodiscard]] IPin& outPin() const { return *m_out_pin; }

    /**
     * @brief Get input pin of the link
     * @return Reference to the Pin
     */
    [[nodiscard]] IPin& inPin() const { return *m_in_pin; }

    /**
     * @brief Get hovering status
     * @return [TRUE] If the link is hovered in the current frame
     */
    [[nodiscard]] bool isHovered() const { return m_hovered; }

    /**
     * @brief Get selected status
     * @return [TRUE] If the link is selected in the current frame
     */
    [[nodiscard]] bool isSelected() const { return m_selected; }

    /**
     * @brief Destroys this link
     */
    void destroy() noexcept;

    /**
     * @brief Returns whether or not this link was marked for deletion
     */
    [[nodiscard]] bool isMarkedForDeletion() const
    {
        return m_marked_for_deletion;
    }

private:
    bool m_marked_for_deletion = false;
    IPin* m_out_pin;
    IPin* m_in_pin;
    bool m_hovered = false;
    bool m_selected = false;
};

} // namespace ImFlow::detail

#endif  // IMNODEFLOW_SRC_LINK_H
