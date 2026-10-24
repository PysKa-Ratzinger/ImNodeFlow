#ifndef IM_NODE_FLOW
#define IM_NODE_FLOW
#pragma once

#include "../src/context_wrapper.h"
#include "ImNodeFlow/fwd.h"

#include <imgui.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

/// NOTE: Do not depend on anything inside the detail namespace as a user of
/// the public API
namespace ImFlow::detail {

// -----------------------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS

class IPin;
class Link;
struct editor_context_t;
struct node_t;
struct pin_base_t;
template <typename T>
struct in_pin_t;
template <typename T>
struct out_pin_t;

} // namespace ImFlow::detail

namespace ImFlow {

// -----------------------------------------------------------------------------------------------------------------
// CLASSES PRE-DEFINITIONS

using right_click_popup_handler_t = std::function<void(std::optional<NodeRef> node)>;

// -----------------------------------------------------------------------------------------------------------------
// NODE BEHAVIOUR INTERFACE

/**
 * @brief Node behaviour strategy.
 *
 * This is what allows the user to customize the node's appearance and
 * behaviour
 */
class INodeBehaviour {
public:
    virtual ~INodeBehaviour() = default;

    /**
     * @brief Called immediately after a node is constructed.
     */
    virtual void setup(NodeRef node);

    /**
     * @brief Called each frame. Responsible for the appearance of the
     * node's inner content
     */
    virtual void draw(NodeRef node) = 0;

    /**
     * @brief Called during a node cloning event. If not implemented or
     * nullptr is returned, the cloning event fails for the corresponding
     * node.
     */
    [[nodiscard]] virtual std::unique_ptr<INodeBehaviour> clone()
    {
        return { };
    }
};

// -----------------------------------------------------------------------------------------------------------------
// PIN'S PROPERTIES

struct PinUID {
    PinUID(uint64_t value) noexcept;

    static PinUID generate() noexcept;
    bool operator==(PinUID const& other) const noexcept;

    uint64_t id;
};

/**
 * @brief Extra pin's style setting
 */
struct PinStyleExtras {
    /// @brief Top and bottom spacing
    ImVec2 padding = ImVec2(3.f, 1.f);
    /// @brief Border and background corner rounding
    float bg_radius = 8.f;
    /// @brief Border thickness
    float border_thickness = 1.f;
    /// @brief Background color
    ImU32 bg_color = IM_COL32(23, 16, 16, 0);
    /// @brief Background color when hovered
    ImU32 bg_hover_color = IM_COL32(100, 100, 255, 70);
    /// @brief Border color
    ImU32 border_color = IM_COL32(255, 255, 255, 0);

    /// @brief Link thickness
    float link_thickness = 2.6f;
    /// @brief Link thickness when dragged
    float link_dragged_thickness = 2.2f;
    /// @brief Link thickness when hovered
    float link_hovered_thickness = 3.5f;
    /// @brief Thickness of the outline of a selected link
    float link_selected_outline_thickness = 0.5f;
    /// @brief Color of the outline of a selected link
    ImU32 outline_color = IM_COL32(80, 20, 255, 200);

    /// @brief Spacing between pin content and socket
    float socket_padding = 6.6f;
};

/**
 * @brief Defines the visual appearance of a pin
 */
class PinStyle {
public:
    constexpr PinStyle(
        ImU32 color,
        int socket_shape,
        float socket_radius,
        float socket_hovered_radius,
        float socket_connected_radius,
        float socket_thickness) noexcept
        : color(color)
        , socket_shape(socket_shape)
        , socket_radius(socket_radius)
        , socket_hovered_radius(socket_hovered_radius)
        , socket_connected_radius(socket_connected_radius)
        , socket_thickness(socket_thickness)
    {
    }

    /// @brief Socket and link color
    ImU32 color;
    /// @brief Socket shape ID
    int socket_shape;
    /// @brief Socket radius
    float socket_radius;
    /// @brief Socket radius when hovered
    float socket_hovered_radius;
    /// @brief Socket radius when connected
    float socket_connected_radius;
    /// @brief Socket outline thickness when empty
    float socket_thickness;
    /// @brief List of less common properties
    PinStyleExtras extra;
};

namespace PinStyles {

    /// @brief Default cyan style
    static constexpr PinStyle cyan = { IM_COL32(87, 155, 185, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default green style
    static constexpr PinStyle green = { IM_COL32(90, 191, 93, 255), 4, 4.f, 4.67f, 4.2f, 1.3f };
    /// @brief Default blue style
    static constexpr PinStyle blue = { IM_COL32(90, 117, 191, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default brown style
    static constexpr PinStyle brown = { IM_COL32(191, 134, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default red style
    static constexpr PinStyle red = { IM_COL32(191, 90, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    /// @brief Default white style
    static constexpr PinStyle white = { IM_COL32(255, 255, 255, 255), 5, 4.f, 4.67f, 4.2f, 1.f };

    static inline constexpr PinStyle default_style = cyan;

}

/**
 * @brief Pins type identifier
 */
enum PinType {
    PinType_Input,
    PinType_Output,
};

/**
 * @brief Strategy to use for input pins regarding multiple connections
 */
enum PinConnStrat {
    /// When a new link is created, all other links connecting to this pin
    /// are deleted.
    PinConnStrat_AllowOnlySingleConnection,

    /// Multiple links are allowed.
    ///
    /// When applied to an input pin, if an accumulate operation is not
    /// provided, it will default to fetch the value from the oldest
    /// connected link.
    ///
    /// The behaviour for an output pin does not change besides allowing
    /// the creation of multiple links.
    PinConnStrat_AllowMultiplePreferOldest,

    /// Multiple links are allowed.
    ///
    /// When applied to an input pin, if an accumulate operation is not
    /// provided, it will be set to fetch the value from the newest
    /// connected link.
    ///
    /// The behaviour for an output pin does not change besides allowing
    /// the creation of multiple links.
    PinConnStrat_AllowMultiplePreferNewest,
};

// -----------------------------------------------------------------------------------------------------------------
// PIN INFO STRUCTURES

/**
 * @brief Description of parameters customizable during pin creation
 */
struct BasePinInfo {
    /// Name of the pin
    std::string name;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections;
    /// Style to apply to the pin
    PinStyle style;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat;
    /// Custom render function
    std::function<void(PinRef<> p)> renderer;
};

/**
 * @brief Description of parameters customizable during pin creation
 */
template <typename T>
struct InPinInfo {
    /// Name of the pin
    std::string name;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections;
    /// Style to apply to the pin
    PinStyle style = PinStyles::default_style;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat_AllowOnlySingleConnection;
    /// Custom render function
    std::function<void(PinRef<> p)> renderer;

    /// Default value the input pin will return when not connected
    /// to any output pin
    T default_value;
    /// When an input pin supports multiple inputs, the pin's value will
    /// be calculated by first initializing it to the default value, and
    /// then calling this accumulator function for every connected pin
    /// (from oldest to newest).
    std::function<T(bool first_pin, T init, T const& incoming_value)>
        accumulate_op;
};

/**
 * @brief Description of parameters customizable during pin creation
 */
template <typename T>
struct OutPinInfo {
    /// Name of the pin
    std::string name;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections;
    /// Style to apply to the pin
    PinStyle style = PinStyles::default_style;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat_AllowMultiplePreferOldest;
    /// Custom render function
    std::function<void(PinRef<> p)> renderer;

    /// Default value the output pin will return before the behaviour has a
    /// chance to change it.
    T default_value;
    /// Behaviour of the pin when a value update is requested
    std::function<T(NodeRef)> behaviour;
};

// -----------------------------------------------------------------------------------------------------------------
// NODE'S PROPERTIES

struct NodeUID {
    NodeUID(uint64_t value) noexcept;

    static NodeUID generate() noexcept;
    bool operator==(NodeUID const& other) const noexcept;

    uint64_t id;

    struct Hash {
        auto operator()(const NodeUID& uid) const -> std::size_t;
    };
};

/**
 * @brief Defines the visual appearance of a node
 */
class NodeStyle {
public:
    constexpr NodeStyle(
        ImU32 header_bg,
        ImColor header_title_color,
        float radius) noexcept
        : header_bg(header_bg)
        , header_title_color(header_title_color)
        , radius(radius)
    {
    }

    /// @brief Body's background color
    ImU32 bg = IM_COL32(55, 64, 75, 255);
    /// @brief Header's background color
    ImU32 header_bg;
    /// @brief Header title color
    ImColor header_title_color;
    /// @brief Border color
    ImU32 border_color = IM_COL32(30, 38, 41, 140);
    /// @brief Border color when selected
    ImU32 border_selected_color = IM_COL32(170, 190, 205, 230);

    /// @brief Body's content padding (Left Top Right Bottom)
    ImVec4 padding = ImVec4(13.7f, 6.f, 13.7f, 2.f);
    /// @brief Edges rounding
    float radius;
    /// @brief Border thickness
    float border_thickness = -1.35f;
    /// @brief Border thickness when selected
    float border_selected_thickness = 2.f;
};

namespace NodeStyles {

    /// @brief Default cyan style
    static constexpr NodeStyle cyan = { IM_COL32(71, 142, 173, 255), ImColor(233, 241, 244, 255), 6.5f };
    /// @brief Default green style
    static constexpr NodeStyle green = { IM_COL32(90, 191, 93, 255), ImColor(233, 241, 244, 255), 3.5f };
    /// @brief Default red style
    static constexpr NodeStyle red = { IM_COL32(191, 90, 90, 255), ImColor(233, 241, 244, 255), 11.f };
    /// @brief Default brown style
    static constexpr NodeStyle brown = { IM_COL32(191, 134, 90, 255), ImColor(233, 241, 244, 255), 6.5f };

    static inline constexpr NodeStyle default_style = cyan;

}

/**
 * @brief User customizable options when creating a new Node
 */
struct NodeInfo {
    /// Node strategy for initialization and rendering
    std::unique_ptr<INodeBehaviour> behaviour;
    /// Style to apply to the node
    NodeStyle style = NodeStyles::default_style;
};

// -----------------------------------------------------------------------------------------------------------------
// HANDLER

/**
 * @brief Grid's the color parameters
 */
struct InfColors {
    /// @brief Background of the grid
    ImU32 background = IM_COL32(33, 41, 45, 255);
    /// @brief Main lines of the grid
    ImU32 grid = IM_COL32(200, 200, 200, 40);
    /// @brief Secondary lines
    ImU32 subGrid = IM_COL32(200, 200, 200, 10);
};

/**
 * @brief ALl the grid's appearance parameters. Sizes + Colors
 */
struct InfStyler {
    /// @brief Size of main grid
    float grid_size = 50.f;
    /// @brief Sub-grid divisions for Node snapping
    float grid_subdivisions = 5.f;
    /// @brief Tightness of gradient links. 1 = linear, >1 concentrates the color transition toward the middle, <1 spreads it out
    float link_gradient_blend = 1.f;
    /// @brief ImNodeFlow colors
    InfColors colors;
};

// -----------------------------------------------------------------------------------------------------------------
// NODE FLOW CONTEXT

/**
 * @brief Main node editor
 * @details Handles the infinite grid, nodes and links. Also handles all the logic.
 */
class ImNodeFlow {
private:
    static int m_instances;

public:
    /**
     * @brief Instantiate a new editor with default name.
     * Editor name will be "FlowGrid + the number of editors"
     */
    ImNodeFlow();

    /**
     * @brief Instantiate a new editor with given name
     * @details Creates a new Node Editor with the given name.
     * @param name Name of the editor
     */
    explicit ImNodeFlow(std::string name);

    // delete copy-ctor
    ImNodeFlow(const ImNodeFlow&) = delete;
    // delete copy-operator
    ImNodeFlow& operator=(const ImNodeFlow&) = delete;

    // move-ctor
    ImNodeFlow(ImNodeFlow&&) noexcept;
    // move-operator
    ImNodeFlow& operator=(ImNodeFlow&&);

    ~ImNodeFlow();

    /**
     * @brief Handler loop
     * @details Main update function. Refreshes all the logic and draws everything. Must be called every frame.
     */
    void update();

    /**
     * @brief Add a node to the grid
     * @param grid_coords Position of the Node in grid coordinates
     * @param node_info Node behaviour customization options
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    NodeRef addNode(const ImVec2& grid_coords, NodeInfo node_info = { });

    /**
     * @brief Add a node to the grid
     * @param screen_coords Position of the Node in screen coordinates
     * @param node_info Node behaviour customization options
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    NodeRef placeNodeAt(const ImVec2& screen_coords, NodeInfo node_info = { });

    /**
     * @brief Add a node to the grid using mouse position
     * @param args Optional arguments to be forwarded to derived class ctor
     * @param node_info Node behaviour customization options
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    NodeRef placeNode(NodeInfo node_info = { });

    /**
     * @brief Add a node whos operation can be defined within a lambda.
     * @tparam L the type of the lambda
     * @param grid_coords the position at which to place the node
     * @param draw_func the draw_func that defines the nodes operation
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    NodeRef addNodeLambda(
        const ImVec2& grid_coords,
        std::function<void(NodeRef)> draw_func);

    /**
     * @brief Add a node to the grid
     * @tparam T Derived class of <INodeBehaviour> to be added
     * @tparam Params types of optional args to forward to derived class ctor
     * @param grid_coords Position of the Node in grid coordinates
     * @param args Arguments to be forwarded to derived class ctor
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    template <typename T, typename... Params>
    NodeRef emplaceNode(const ImVec2& grid_coords, Params&&... args);

    /**
     * @brief Add a node to the grid
     * @tparam T Derived class of <INodeBehaviour> to be added
     * @tparam Params types of optional args to forward to derived class ctor
     * @param screen_coords Position of the Node in screen coordinates
     * @param args Arguments to be forwarded to derived class ctor
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    template <typename T, typename... Params>
    NodeRef emplaceNodeAt(const ImVec2& screen_coords, Params&&... args);

    /**
     * @brief Add a node to the grid using mouse position
     * @tparam T Derived class of <Node> to be added
     * @tparam Params types of optional args to forward to derived class ctor
     * @param args Arguments to be forwarded to derived class ctor
     * @return Reference to the newly added node. Do not save this
     * reference.
     */
    template <typename T, typename... Params>
    NodeRef emplaceNodeOnMouse(Params&&... args);

    /**
     * @brief Creates a link between two pins
     * @param source_pin Reference to the input pin
     * @param target_pint Reference to the output pin
     */
    void createLink(PinRef<> source_pin, PinRef<> target_pin);

    /**
     * @brief Pop-up when link is "dropped"
     * @details Sets the content of a pop-up that can be displayed when dragging a link in the open instead of onto another pin.
     * @details If "key = ImGuiKey_None" the pop-up will always open when a link is dropped.
     * @param content Function or Lambda containing only the contents of the pop-up and the subsequent logic
     * @param key Optional key required in order to open the pop-up
     */
    void droppedLinkPopUpContent(
        std::function<void(PinRef<> dragged)> content,
        ImGuiKey key = ImGuiKey_None);

    /**
     * @brief Pop-up when right-clicking
     * @details Sets the content of a pop-up that can be displayed when right-clicking on the grid.
     * @param content Function or Lambda containing only the contents of the pop-up and the subsequent logic
     */
    void rightClickPopUpContent(right_click_popup_handler_t content);

    /**
     * @brief <BR>Pop-up when drag_dropping
     * @details Sets the node pos when drag dropping a node to the grid
     * @param content Function or Lambda containing only the pos of the mouse when clicked
     */
    void dragDropTarget(std::function<void(const ImVec2&)> content);

    /**
     * @brief Get mouse clicking status
     * @param The mouse button to query
     * @return [TRUE] if mouse is clicked and click hasn't been consumed
     */
    [[nodiscard]] bool getSingleUseClick(ImGuiMouseButton button) const;

    /**
     * @brief Consume the click for the given frame
     * @param button The mouse button to query
     */
    void consumeSingleUseClick(ImGuiMouseButton button);

    /**
     * @brief Get editor's name
     * @return Const reference to editor's name
     */
    [[nodiscard]] const std::string& getName() const;

    /**
     * @brief Get editor's position
     * @return Const reference to editor's position in screen coordinates
     */
    [[nodiscard]] const ImVec2& getPos() const;

    /**
     * @brief Get editor's grid scroll
     * @details Scroll is the offset from the origin of the grid, changes while navigating the grid.
     * @return Const reference to editor's grid scroll
     */
    const ImVec2& getScroll() const;

    /**
     * @brief Sets the context's scroll to a user specified value
     */
    void setScroll(ImVec2 scroll);

    /**
     * @brief Sets the context's scale to a user specified value
     */
    void setScale(float scale);

    /**
     * @brief Get the scale adjusted screen space mouse delta, needed for dragging
     *
     * @return scale adjusted mouse delta.
     */
    ImVec2 getScreenSpaceDelta() const;

    /**
     * @brief Get nodes count
     * @return Number of nodes present in the editor
     */
    uint64_t getNodesCount() const;

    /**
     * @brief Get zooming viewport
     * @return Const reference to editor's internal viewport for zoom support
     */
    ContainedContext& getGrid();

    /**
     * @brief Get dragging status
     * @return [TRUE] if a Node is being dragged around the grid
     */
    [[nodiscard]] bool isNodeDragged() const;

    /**
     * @brief Get current style
     * @return Reference to style variables
     */
    const InfStyler& getStyle() const;

    /**
     * @brief Modify current style
     */
    void setStyle(InfStyler style);

    /**
     * @brief Set editor's size
     * @param size Editor's size. Set to (0, 0) to auto-fit.
     */
    void setSize(const ImVec2& size);

    /**
     * @brief Set dragging status
     * @param state New dragging state
     *
     * The new state will only be updated one at the start of each frame.
     */
    void draggingNode(bool state);

    /**
     * @brief Set what pin is being hovered
     * @param hovering Pointer to the hovered pin
     * @internal For internal use only
     */
    void hovering(detail::IPin* hovering);

    /**
     * @brief Set what pin is being dragged
     * @param dragged Pointer to the dragged pin
     * @internal For internal use only
     */
    void dragging(detail::IPin* dragged);

    /**
     * @brief Set what node is being hovered
     * @param hovering Pointer to the hovered node
     * @internal For internal use only
     */
    void hoveredNode(detail::node_t* hovering);

    /**
     * @brief Convert coordinates from screen to grid
     * @param p Point in screen coordinates to be converted
     * @return Point in grid's coordinates
     */
    ImVec2 screen2grid(const ImVec2& p);

    /**
     * @brief Convert coordinates from grid to screen
     * @param p Point in grid's coordinates to be converted
     * @return Point in screen coordinates
     */
    ImVec2 grid2screen(const ImVec2& p);

    /**
     * @brief Check if mouse is on selected node
     * @return [TRUE] if the mouse is hovering a selected node
     */
    bool mouse_on_selected_node();

    /**
     * @brief Check if mouse is on a free point on the grid
     * @return [TRUE] if the mouse is not hovering a node or a link
     */
    bool mouse_on_free_space();

private:
    std::unique_ptr<detail::editor_context_t> m_impl;
};

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

/**
 * @brief Parent class for custom nodes
 * @details Main class from which custom nodes can be created. All interactions with the main grid are handled internally.
 */
template <bool TIsConst>
class NodeRefImpl {
public:
    using underlying_type = std::conditional_t<TIsConst, const detail::node_t, detail::node_t>;

    /**
     * @brief NodeRef's ctor
     * @param internal_node Reference to the referred to internal node
     */
    explicit NodeRefImpl(underlying_type& internal_node);

    /**
     * @brief Main loop of the node
     * @details Updates position, hovering and selected status, and renders the node. Must be called each frame.
     */
    void update()
        requires(!TIsConst);

    /**
     * @brief Add an Input to the node
     * @details Will add an Input pin to the node with the given name and data type.
     * In this case the name of the pin will also be its UID.
     *The UID must be unique only in the context of the current node's inputs.

     * @tparam T Type of the data the pin will handle
     * @param pin_info User given options to the InPin during construction
     * @return Reference to the newly added pin. Do not save this
     * reference. Use the reference to query the pin's UID and save that
     * instead.
     */
    template <typename T>
    InPinRef<T, TIsConst> addIN(InPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Add an Input to the node
     * @details Will add an Input pin to the node with the given name and data type.
     * The UID must be unique only in the context of the current node's inputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info User given options to the InPin during construction
     * @return Reference to the newly added pin. Do not save this
     * reference. Use the reference to query the pin's UID and save that
     * instead.
     */
    template <typename T>
    InPinRef<T, TIsConst> addIN(const PinUID& uid, InPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Remove input pin
     * @param uid Unique identifier of the pin
     */
    void dropIN(const PinUID& uid)
        requires(!TIsConst);

    /**
     * @brief Remove all input pin's with the associated name
     * @param name Pin's name filter
     */
    void dropIN(const char* name)
        requires(!TIsConst);

    /**
     * @brief Show a temporary input pin
     * @details Will show an input pin with the given name.
     * The pin is created the first time showIN is called and kept alive as long as showIN is called each frame.
     * In this case the name of the pin will also be its UID.
     * The UID must be unique only in the context of the current node's inputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param pin_info User given options to the InPin during construction
     * @return Const reference to the value of the connected link for the current frame of defReturn
     */
    template <typename T>
    const T& showIN(InPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Show a temporary input pin
     * @details Will show an input pin with the given name and UID.
     * The pin is created the first time showIN_uid is called and kept alive as long as showIN_uid is called each frame.
     * The UID must be unique only in the context of the current node's inputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info User given options to the InPin during construction
     * @return Const reference to the value of the connected link for the current frame of defReturn
     */
    template <typename T>
    const T& showIN(const PinUID& uid, InPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Add an Output to the node
     * @details Must be called in the node constructor. WIll add an Output pin to the node with the given name and data type.
     * In this case the name of the pin will also be its UID.
     * The UID must be unique only in the context of the current node's outputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param pin_info User given options to the InPin during construction
     * @return Reference to the newly added pin. Do not save this
     * reference. Use the reference to query the pin's UID and save that
     * instead.
     */
    template <typename T>
    OutPinRef<T, TIsConst> addOUT(OutPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Add an Output to the node
     * @details Must be called in the node constructor. WIll add an Output pin to the node with the given name and data type.
     * The UID must be unique only in the context of the current node's outputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info User given options to the InPin during construction
     * @return Reference to the newly added pin. Do not save this
     * reference. Use the reference to query the pin's UID and save that
     * instead.
     */
    template <typename T>
    OutPinRef<T, TIsConst> addOUT(const PinUID& uid, OutPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Remove output pin
     * @param uid Unique identifier of the pin
     */
    void dropOUT(const PinUID& uid)
        requires(!TIsConst);

    /**
     * @brief Remove all output pins with a matching name
     * @param name The name to filter for
     */
    void dropOUT(const char* name)
        requires(!TIsConst);

    /**
     * @brief Show a temporary output pin
     * @details Will show an output pin with the given name.
     * The pin is created the first time showOUT is called and kept alive as long as showOUT is called each frame.
     * In this case the name of the pin will also be its UID.
     * The UID must be unique only in the context of the current node's outputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param pin_info User given options to the InPin during construction
     */
    template <typename T>
    void showOUT(OutPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Show a temporary output pin
     * @details Will show an output pin with the given name.
     * The pin is created the first time showOUT_uid is called and kept alive as long as showOUT_uid is called each frame.
     * The UID must be unique only in the context of the current node's outputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info User given options to the InPin during construction
     */
    template <typename T>
    void showOUT(const PinUID& uid, OutPinInfo<T> pin_info)
        requires(!TIsConst);

    /**
     * @brief Get generic reference to input pin
     * @param uid Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    std::optional<PinRef<TIsConst>> inPin(const PinUID& uid);

    /**
     * @brief Get generic reference to input pin with the specified name
     * @param uid Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    std::optional<PinRef<TIsConst>> inPin(const char* name);

    /**
     * @brief Get generic reference to output pin
     * @param uid Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    std::optional<PinRef<TIsConst>> outPin(const PinUID& uid);

    /**
     * @brief Get generic reference to output pin of the same name
     * @param name Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    std::optional<PinRef<TIsConst>> outPin(const char* name);

    /**
     * @brief Get generic reference to input pin
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    template <typename T>
    std::optional<InPinRef<T, TIsConst>>
    inPin(const PinUID& uid);

    /**
     * @brief Get generic reference to input pin with the specified name
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    template <typename T>
    std::optional<InPinRef<T, TIsConst>>
    inPin(const char* name);

    /**
     * @brief Get generic reference to output pin
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    template <typename T>
    std::optional<OutPinRef<T, TIsConst>>
    outPin(const PinUID& uid);

    /**
     * @brief Get generic reference to output pin of the same name
     * @tparam T Type of the data the pin will handle
     * @param name Unique identifier of the pin
     * @return Pointer to the pin, if it exists
     */
    template <typename T>
    std::optional<OutPinRef<T, TIsConst>>
    outPin(const char* name);

    /**
     * @brief Delete itself
     */
    void destroy()
        requires(!TIsConst);

    /*
     * @brief Get if node must be deleted
     */
    [[nodiscard]] bool toDestroy() const;

    /**
     * @brief Get hovered status
     * @return [TRUE] if the mouse is hovering the node
     */
    bool isHovered() const;

    /**
     * @brief Get node's UID
     * @return NodeRef's unique identifier
     */
    [[nodiscard]] NodeUID getUID() const;

    /**
     * @brief Get node name
     * @return Const reference to the node's name
     */
    const std::string& getName() const;

    /**
     * @brief Get node size
     * @return Const reference to the node's size
     */
    const ImVec2& getSize() const;

    /**
     * @brief Get node size
     * @return Const reference to the node's size
     */
    const ImVec2& getFullSize() const;

    /**
     * @brief Get node position
     * @return Const reference to the node's position
     */
    const ImVec2& getPos() const;

    /**
     * @brief Get node position
     * @return Const reference to the node's position
     */
    ImVec2 getTLPos() const;

    /**
     * @brief Get node's style
     * @return Shared pointer to the node's style
     */
    const NodeStyle& getStyle() const;

    /**
     * @brief Get selected status
     * @return [TRUE] if the node is selected
     */
    [[nodiscard]] bool isSelected() const;

    /**
     * @brief Get dragged status
     * @return [TRUE] if the node is being dragged
     */
    [[nodiscard]] bool isDragged() const;

    /**
     * @brief Get Indestructable status
     * @return [TRUE] if the node is Indestructable
     */
    [[nodiscard]] bool isIndestructable() const;

    /**
     * @brief Set node's uid
     * @param uid Node's unique identifier
     */
    NodeRefImpl& setUID(NodeUID uid)
        requires(!TIsConst);

    /**
     * @brief Set node's name
     * @param name New title
     */
    NodeRefImpl& setTitle(const std::string& title)
        requires(!TIsConst);

    /**
     * @brief Set node's position
     * @param pos Position in grid coordinates
     */
    NodeRefImpl& setPos(const ImVec2& pos)
        requires(!TIsConst);

    /**
     * @brief Set node's style
     * @param style New style
     */
    NodeRefImpl& setStyle(NodeStyle style)
        requires(!TIsConst);

    /**
     * @brief Set node's style
     * @param style New style
     */
    NodeRefImpl& setIndestructable(bool indestruct)
        requires(!TIsConst);

    /**
     * @brief Set selected status
     * @param state New selected state
     *
     * Status only updates when updatePublicStatus() is called
     */
    NodeRefImpl& setSelected(bool state)
        requires(!TIsConst);

    /**
     * @brief Set provisionally selected status
     * @param state New provisionally selected state
     *
     * Status only updates when updatePublicStatus() is called
     */
    NodeRefImpl& setProvisionallySelected(bool state)
        requires(!TIsConst);

    /**
     * @brief Update the isSelected status of the node
     */
    void updatePublicStatus()
        requires(!TIsConst);

    /**
     * @brief Have every pin calculate their next output value
     */
    void calculate()
        requires(!TIsConst);

    /**
     * @brief Have every pin commit their calculated values as their new
     * output value
     */
    void prime()
        requires(!TIsConst);

    /**
     * @internal For internal use only
     */
    operator underlying_type&()
    {
        return m_node.get();
    }

    [[nodiscard]] constexpr bool operator==(const NodeRefImpl& other) const noexcept
    {
        return &m_node.get() == &other.m_node.get();
    }

private:
    std::reference_wrapper<underlying_type> m_node;
};

extern template class NodeRefImpl<false>;
extern template class NodeRefImpl<true>;

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
