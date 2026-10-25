#ifndef IM_NODE_FLOW
#define IM_NODE_FLOW
#pragma once

#include "ImNodeFlow/PinStyle.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/PinUID.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include <imgui.h>

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace ImFlow {

// -----------------------------------------------------------------------------------------------------------------
// PINS

/**
 * @brief Generic base class for pins
 */
template <bool TIsConst>
class PinRef {
public:
    using underlying_type = std::conditional_t<
        TIsConst,
        const detail::IPin,
        detail::IPin>;

    explicit PinRef(underlying_type& pin)
        : m_pin(pin)
    {
    }

    virtual ~PinRef() noexcept = default;

    /**
     * @brief Safely casts a base Pin into the underlying InPin, if
     * possible. Crashes the program otherwise.
     *
     * @return Reference to the underlying InPin<T> if possible. Crash
     * otherwise.
     */
    template <typename T>
    InPinRef<T, TIsConst> toIn();

    /**
     * @brief Safely casts a base Pin into the underlying OutPin, if
     * possible. Crashes the program otherwise.
     *
     * @return Reference to the underlying OutPin<T> if possible. Crash
     * otherwise.
     */
    template <typename T>
    OutPinRef<T, TIsConst> toOut();

    /**
     * @brief Main loop of the pin
     * @details Updates position, hovering and dragging status, and renders the pin. Must be called each frame.
     * @param out_hovering When not null, it receives the value of whether the
     * mouse is hovering over the pin
     */
    void update(bool* out_hovering)
        requires(!TIsConst);

    /**
     * @brief Draw default pin's socket
     * @param out_hovering_socket When not null, receives whether the mouse is
     * hovering over the pin's socket
     */
    void drawSocket(bool* out_hovering_socket);

    /**
     * @brief Draw default pin's decoration (border, bg, and hover overlay)
     */
    void drawDecoration();

    /**
     * @brief Custom render function to override Pin appearance
     * @param r Function or lambda expression with new ImGui rendering
     */
    PinRef& renderer(std::function<void(PinRef<> p)> r)
        requires(!TIsConst);

    /**
     * @brief Creates a new link between this pin and another pin
     * @param other Reference of the pin we want to connect with
     */
    void linkTo(PinRef<TIsConst> other)
        requires(!TIsConst);

    /**
     * @brief Delete all link reference
     */
    void deleteAllLinks()
        requires(!TIsConst);

    /**
     * @brief Handles the request for when a link is trying to connect
     * @param link Pointer to the link making the request
     *
     * @internal This function is for internal use only. Do not call this
     * function as an API user.
     */
    void addLink(detail::Link* link)
        requires(!TIsConst);

    /**
     * @brief Delete a single link reference
     *
     * @internal This function is for internal use only. Do not call this
     * function as an API user.
     */
    void removeLink(detail::Link* link)
        requires(!TIsConst);

    /**
     * @brief Get connected status
     * @return [TRUE] if the pin is connected
     */
    [[nodiscard]] bool isConnected() const;

    /**
     * @brief Get pin's UID
     * @return Unique identifier of the pin
     */
    [[nodiscard]] PinUID getUid() const;

    /**
     * @brief Get pin's name
     * @return Const reference to pin's name
     */
    [[nodiscard]] const std::string& getName() const;

    /**
     * @brief Get pin's position
     * @return Const reference to pin's position in grid coordinates
     */
    [[nodiscard]] const ImVec2& getPos() const;

    /**
     * @brief Get pin's hit-box size
     * @return Const reference to pin's hit-box size
     */
    [[nodiscard]] const ImVec2& getSize() const;

    /**
     * @brief Get pin's parent node
     * @return Generic type pointer to pin's parent node. (Node that contains it)
     */
    [[nodiscard]] NodeRefImpl<TIsConst> getParent() const;

    /**
     * @brief Get pin's type
     * @return The pin type. Either Input or Output
     */
    [[nodiscard]] PinType getType() const;

    /**
     * @brief Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] uint64_t getTypeID() const;

    /**
     * @brief Get pin's style
     * @return Reference to pin's style
     */
    const PinStyle& getStyle() const;

    /**
     * @brief Sets pin's style
     */
    void setStyle(PinStyle style)
        requires(!TIsConst);

    /**
     * @brief Specify if connections from an output on the same node are allowed
     * @param state New state of the flag
     */
    void allowSameNodeConnections(bool state)
        requires(!TIsConst);

    /**
     * @brief Specify if connections from an output on the same node are allowed
     * @param state New state of the flag
     */
    [[nodiscard]] bool allowsSameNodeConnections() const;

    /**
     * @brief Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    ImVec2 pinPoint() const;

    /**
     * @brief Calculate pin's width pre-rendering
     * @return The with of the pin once it will be rendered
     */
    float calcWidth() const;

    /**
     * @brief Set pin's position
     * @param pos Position in screen coordinates
     */
    void setPos(ImVec2 pos)
        requires(!TIsConst);

    /**
     * @brief Get socket hit bounds for interaction
     * @param expand_radius Optional radius to expand the socket hitbox (default uses socket_hovered_radius)
     * @return Rectangle bounds for socket interaction
     */
    std::pair<ImVec2, ImVec2>
    getSocketHitBounds(float expand_radius = -1.0f);

    /**
     * @brief Enable/disable automatic socket hitbox extension
     * @param enabled If true, socket area will be included in pin hitbox
     */
    void setSocketHitboxEnabled(bool enabled)
        requires(!TIsConst);

    /**
     * @brief When the context calls this method, the pins should calculate
     * their next value, but not apply it. This allows every pin, during
     * the calculation step, to query what every other pin's output value
     * was in the closing iteration step.
     * @internal This function is for internal use only
     */
    void calculate()
        requires(!TIsConst);

    /**
     * @brief When the context calls this method, the pins should update
     * their outputs to the computed value in calculate().
     *
     * During this step, pin's are no longer allowed to query any other
     * part of the flow graph, since their state is undefined.
     * @internal This function is for internal use only
     */
    void prime()
        requires(!TIsConst);

    /**
     * @internal For internal use only
     */
    operator underlying_type&()
    {
        return m_pin.get();
    }

protected:
    std::reference_wrapper<underlying_type> m_pin;
};

extern template class PinRef<false>;
extern template class PinRef<true>;

/**
 * @brief Input specific pin
 * @details Derived from the generic class Pin. The input pin owns the link pointer.
 * @tparam T Data type handled by the pin
 */
template <class T, bool TIsConst>
class InPinRef : public PinRef<TIsConst> {
public:
    using underlyin_type = std::conditional_t<
        TIsConst,
        const detail::in_pin_t<T>,
        detail::in_pin_t<T>>;

    /**
     * @brief Input pin constructor
     * @param ctx Reference to the ImNodeFlow context
     * @param name Name of the pin
     * @param parent Pointer to the Node containing the pin
     * @param defReturn Default return value when the pin is not connected
     * @param style Style of the pin
     */
    explicit InPinRef(underlyin_type& pin)
        : PinRef<TIsConst>(pin)
        , m_in_pin(pin)
    {
    }

    /**
     * @brief Get value carried by the connected link
     * @return Reference to the value of the connected OutPin. Or the default value if not connected
     */
    const T& value() const;

private:
    std::reference_wrapper<underlyin_type> m_in_pin;
};

/**
 * @brief Output specific pin
 * @details Derived from the generic class Pin. The output pin handles the logic.
 * @tparam T Data type handled by the pin
 */
template <class T, bool TIsConst>
class OutPinRef final : public PinRef<TIsConst> {
public:
    using underlying_type = std::conditional_t<
        TIsConst,
        const detail::out_pin_t<T>,
        detail::out_pin_t<T>>;

    explicit OutPinRef(
        underlying_type& out_pin)
        : PinRef<TIsConst>(out_pin)
        , m_out_pin(out_pin)
    {
    }

    ~OutPinRef() override = default;

    /**
     * @brief Get output value
     * @return Const reference to the internal value of the pin
     */
    const T& value() const;

    /**
     * @brief Set logic to calculate output value
     * @details Used to define the pin behaviour. This is what gets the data from the parent's inputs, and applies the needed logic.
     * @param func Function or lambda expression used to calculate output value
     */
    OutPinRef<T, TIsConst>& behaviour(std::function<T(NodeRef)> func)
        requires(!TIsConst);

private:
    std::reference_wrapper<underlying_type> m_out_pin;
};

} // namespace ImFlow

#include "../src/ImNodeFlow.inl"

#endif
