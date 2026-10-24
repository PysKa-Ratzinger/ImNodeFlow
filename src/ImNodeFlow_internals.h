#ifndef IM_NODE_FLOW_INTERNALS
#define IM_NODE_FLOW_INTERNALS
#pragma once

#include "ImNodeFlow.h"

#include "./context_wrapper.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ImFlow::detail {

// -----------------------------------------------------------------------------------------------------------------
// PIN INTERFACE

class IPin {
public:
    virtual ~IPin() = default;

    [[nodiscard]] virtual uint64_t getTypeID() const = 0;
    virtual pin_base_t& toBase() = 0;
    [[nodiscard]] virtual pin_base_t const& toBase() const = 0;
    virtual void calculate() = 0;
    virtual void prime() = 0;

    template <typename T>
    in_pin_t<T>& toIn();
    template <typename T>
    in_pin_t<T> const& toIn() const;

    template <typename T>
    out_pin_t<T>& toOut();
    template <typename T>
    out_pin_t<T> const& toOut() const;
};

// -----------------------------------------------------------------------------------------------------------------
// LINK

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

// -----------------------------------------------------------------------------------------------------------------
// DYNAMIC PIN

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

struct editor_context_t {
    std::string name;
    ContainedContext context;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } singleUseClicks;

    std::unordered_map<NodeUID, node_t, NodeUID::Hash>
        nodes;
    std::vector<std::unique_ptr<Link>> links;

    std::function<void(PinRef<> dragged)> droppedLinkPopUp;
    ImGuiKey droppedLinkPupUpComboKey = ImGuiKey_None;
    IPin* droppedLinkLeft = nullptr;
    right_click_popup_handler_t rightClickPopUp;
    std::function<void(const ImVec2&)> dragdropTarget;
    node_t* hoveredNodeAux = nullptr;

    node_t* hoveredNode = nullptr;
    bool draggingNode = false, draggingNodeNext = false;
    IPin* hovering = nullptr;
    IPin* dragOut = nullptr;

    InfStyler style;

    struct SelectionBox {
        bool active = false;
        ImVec2 selectionStart;
        ImU32 selectionColor = IM_COL32(100, 150, 255, 50);
        ImU32 borderColor = IM_COL32(100, 150, 255, 200);
    };

    SelectionBox box;

    bool draggingCanvas = false;
};

struct node_t {
    std::reference_wrapper<ImNodeFlow> ctx;

    NodeUID uid = 0;
    std::string title;
    ImVec2 pos;
    ImVec2 posTarget;
    ImVec2 size;
    ImVec2 fullSize;
    NodeStyle style;
    bool selected = false;
    bool selectedNext = false;
    bool provisionallySelected = false;
    bool provisionallySelectedNext = false;
    bool dragged = false;
    bool destroyed = false;
    bool indestructable = false;

    std::unique_ptr<INodeBehaviour> behaviour;

    std::vector<std::unique_ptr<IPin>> ins;
    std::vector<DynamicPin> dynamicIns;
    std::vector<std::unique_ptr<IPin>> outs;
    std::vector<DynamicPin> dynamicOuts;
};

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

struct pin_base_t {
    std::reference_wrapper<ImNodeFlow> ctx;
    PinUID uid;
    std::string name;
    ImVec2 pos = ImVec2(0.f, 0.f);
    ImVec2 size = ImVec2(0.f, 0.f);
    PinType type;
    detail::node_t* parent = nullptr;
    PinStyle style;
    PinConnStrat strat;
    bool allowSelfConnection = false;
    bool socketHitboxEnabled = true;
    std::function<void(PinRef<> p)> renderer;
    std::vector<PinConnectionHolder> links { };
};

template <typename T>
struct in_pin_t final : public IPin {
    pin_base_t base;
    T emptyVal;
    T val;
    T valNext;
    std::function<T(bool first_pin, T init, T const& incoming_value)>
        accumulatorOp;

    in_pin_t(
        ImNodeFlow& ctx,
        PinUID uid,
        detail::node_t& parent,
        InPinInfo<T> pin_info);

    pin_base_t& toBase() final;
    [[nodiscard]] pin_base_t const& toBase() const final;
    [[nodiscard]] uint64_t getTypeID() const final;
    void calculate() final;
    void prime() final;
};

template <typename T>
struct out_pin_t final : public IPin {
    pin_base_t base;
    std::function<T(NodeRef)> behaviour;
    T val;
    T valNext;

    out_pin_t(
        ImNodeFlow& ctx,
        PinUID uid,
        detail::node_t& parent,
        OutPinInfo<T> pin_info);

    pin_base_t& toBase() final;
    [[nodiscard]] pin_base_t const& toBase() const final;
    [[nodiscard]] uint64_t getTypeID() const final;
    void calculate() final;
    void prime() final;
};

} // namespace ImFlow::detail

#endif // IM_NODE_FLOW_INTERNALS
