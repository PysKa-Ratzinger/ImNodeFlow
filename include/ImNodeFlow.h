#ifndef IM_NODE_FLOW
#define IM_NODE_FLOW
#pragma once

#include "../src/context_wrapper.h"
#include "../src/imgui_bezier_math.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ImFlow {

namespace detail {

    /**
     * @brief Draw a sensible bezier between two points
     * @param p1 Starting point
     * @param p2 Ending point
     * @param color Color of the curve
     * @param thickness Thickness of the curve
     */
    inline static void
    smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness);

    /**
     * @brief Draw a sensible bezier whose color blends from one end to the other
     * @param p1 Starting point
     * @param p2 Ending point
     * @param color1 Color at the starting point
     * @param color2 Color at the ending point
     * @param thickness Thickness of the curve
     * @param blend Tightness of the color transition. 1 = linear, >1 concentrates it toward the middle, <1 spreads it
     */
    inline static void smart_bezier_gradient(
        const ImVec2& p1,
        const ImVec2& p2,
        ImU32 color1,
        ImU32 color2,
        float thickness,
        float blend = 1.f);

    /**
     * @brief Collider checker for smart_bezier
     * @details Projects the point "p" orthogonally onto the bezier curve and
     *	    checks if the distance is less than the given radius.
     * @param p Point to be tested
     * @param p1 Starting point of smart_bezier
     * @param p2 Ending point of smart_bezier
     * @param radius Lateral width of the hit box
     * @return [TRUE] if "p" is inside the collider
     *
     * Intended to be used in union with smart_bezier();
     */
    inline static bool smart_bezier_collider(
        const ImVec2& p,
        const ImVec2& p1,
        const ImVec2& p2,
        float radius);

    /**
     * @brief Returns a locally unique identifier for a specified type
     *
     * This allows pins to connect only to pins of the same type
     */
    template <typename T>
    inline static uint64_t get_type_id();

} // namespace detail

// -----------------------------------------------------------------------------------------------------------------
// CLASSES PRE-DEFINITIONS

template <typename T>
class InPin;
template <typename T>
class OutPin;
class Pin;
class BaseNode;
class ImNodeFlow;

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
        float socket_thickness)
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

    static constexpr PinStyle cyan = { IM_COL32(87, 155, 185, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    static constexpr PinStyle green = { IM_COL32(90, 191, 93, 255), 4, 4.f, 4.67f, 4.2f, 1.3f };
    static constexpr PinStyle blue = { IM_COL32(90, 117, 191, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    static constexpr PinStyle brown = { IM_COL32(191, 134, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    static constexpr PinStyle red = { IM_COL32(191, 90, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f };
    static constexpr PinStyle white = { IM_COL32(255, 255, 255, 255), 5, 4.f, 4.67f, 4.2f, 1.f };

    static inline constexpr PinStyle default_style = cyan;

};

/**
 * @brief Pins type identifier
 */
enum PinType { PinType_Input,
    PinType_Output };

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

/**
 * @brief Description of parameters customizable during pin creation
 */
struct BasePinInfo {
    /// Name of the pin
    std::string name;
    /// Whether it's an input pin or an output pin
    PinType kind;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections = false;
    /// Style to apply to the pin
    PinStyle style = PinStyles::cyan;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat_AllowMultiplePreferOldest;
};

/**
 * @brief Description of parameters customizable during pin creation
 */
template <typename T>
struct InPinInfo {
    /// Name of the pin
    std::string name;
    /// Default value the input pin will return when not connected
    /// to any output pin
    T default_value;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections = false;
    /// Style to apply to the pin
    PinStyle style = PinStyles::cyan;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat_AllowOnlySingleConnection;
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
    /// Behaviour of the pin when a value update is requested
    std::function<T()> behaviour;
    /// Whether links coming from the same node are allowed
    bool allow_same_node_connections = true;
    /// Style to apply to the pin
    PinStyle style = PinStyles::red;
    /// Strategy to use regarding multiple link connections
    PinConnStrat strat = PinConnStrat_AllowMultiplePreferOldest;
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
        float radius)
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

    static constexpr NodeStyle cyan = { IM_COL32(71, 142, 173, 255), ImColor(233, 241, 244, 255), 6.5f };
    static constexpr NodeStyle green = { IM_COL32(90, 191, 93, 255), ImColor(233, 241, 244, 255), 3.5f };
    static constexpr NodeStyle red = { IM_COL32(191, 90, 90, 255), ImColor(233, 241, 244, 255), 11.f };
    static constexpr NodeStyle brown = { IM_COL32(191, 134, 90, 255), ImColor(233, 241, 244, 255), 6.5f };

    static inline constexpr NodeStyle default_style = cyan;

}

// -----------------------------------------------------------------------------------------------------------------
// DYNAMIC PIN

/**
 * @brief A dynamic pin is a pin with a temporary lifetime, and will be marked
 * for deletion if not refreshed before every update
 */
class DynamicPin {
public:
    explicit DynamicPin(std::unique_ptr<Pin> pin);

    void refresh();
    void update();
    Pin& pin();
    const Pin& pin() const;
    [[nodiscard]] bool isStale() const;

    std::unique_ptr<Pin> m_pin;
    uint32_t m_ttl = 1;
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
     * @param inf Pointer to the Handler that contains the Link
     */
    explicit Link(Pin& out_pin, Pin& in_pin) noexcept;

    /**
     * @brief Link destructor
     */
    ~Link() noexcept;

    /**
     * @brief Looping function to update the Link
     * @details Draws the Link and updates Hovering and Selected status.
     */
    void update(ImNodeFlow& ctx);

    /**
     * @brief Get output pin of the link
     * @return Reference to the Pin
     */
    [[nodiscard]] Pin& outPin() const { return *m_out_pin; }

    /**
     * @brief Get input pin of the link
     * @return Reference to the Pin
     */
    [[nodiscard]] Pin& inPin() const { return *m_in_pin; }

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
     * @brief Disconnects this edge from a pin, if connected to it
     * If the link is connected to this pin, the link will be marked for
     * deletion after disconnection
     */
    void disconnect(Pin* pin);

    /**
     * @brief Destroys this link
     */
    void destroy();

    /**
     * @brief Returns whether or not this link was marked for deletion
     */
    [[nodiscard]] bool isMarkedForDeletion() const
    {
        return m_marked_for_deletion;
    }

private:
    bool m_marked_for_deletion = false;
    Pin* m_out_pin;
    Pin* m_in_pin;
    bool m_hovered = false;
    bool m_selected = false;
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
    ImNodeFlow()
        : ImNodeFlow("FlowGrid" + std::to_string(m_instances))
    {
    }

    /**
     * @brief Instantiate a new editor with given name
     * @details Creates a new Node Editor with the given name.
     * @param name Name of the editor
     */
    explicit ImNodeFlow(std::string name)
        : m_name(std::move(name))
    {
        m_instances++;
        m_context.config().extra_window_wrapper = true;
        m_context.config().color = m_style.colors.background;
    }

    /**
     * @brief Handler loop
     * @details Main update function. Refreshes all the logic and draws everything. Must be called every frame.
     */
    void update();

    /**
     * @brief Add a node to the grid
     * @param pos Position of the Node in grid coordinates
     * @return Shared pointer of the newly added node
     */
    BaseNode& addNode(const ImVec2& pos);

    /**
     * @brief Add a node to the grid
     * @param pos Position of the Node in screen coordinates
     * @return Shared pointer to the newly added node
     */
    BaseNode& placeNodeAt(const ImVec2& pos);

    /**
     * @brief Add a node to the grid using mouse position
     * @return Shared pointer to the newly added node
     */
    BaseNode& placeNode();

    /**
     * @brief Creates a link between two pins
     * @param source_pin Reference to the input pin
     * @param target_pint Reference to the output pin
     */
    void createLink(Pin& source_pin, Pin& target_pin);

    /**
     * @brief Pop-up when link is "dropped"
     * @details Sets the content of a pop-up that can be displayed when dragging a link in the open instead of onto another pin.
     * @details If "key = ImGuiKey_None" the pop-up will always open when a link is dropped.
     * @param content Function or Lambda containing only the contents of the pop-up and the subsequent logic
     * @param key Optional key required in order to open the pop-up
     */
    void droppedLinkPopUpContent(
        std::function<void(Pin* dragged)> content,
        ImGuiKey key = ImGuiKey_None)
    {
        m_droppedLinkPopUp = std::move(content);
        m_droppedLinkPupUpComboKey = key;
    }

    /**
     * @brief Pop-up when right-clicking
     * @details Sets the content of a pop-up that can be displayed when right-clicking on the grid.
     * @param content Function or Lambda containing only the contents of the pop-up and the subsequent logic
     */
    void rightClickPopUpContent(std::function<void(BaseNode* node)> content)
    {
        m_rightClickPopUp = std::move(content);
    }

    /**
     * @brief <BR>Pop-up when drag_dropping
     * @details Sets the node pos when drag dropping a node to the grid
     * @param content Function or Lambda containing only the pos of the mouse when clicked
     */
    void dragDropTarget(std::function<void(const ImVec2&)> content)
    {
        m_dragdropTarget = std::move(content);
    }

    /**
     * @brief Get mouse clicking status
     * @return [TRUE] if mouse is clicked and click hasn't been consumed
     */
    [[nodiscard]] bool getSingleUseClick(ImGuiMouseButton button) const
    {
        switch (button) {
        case ImGuiMouseButton_Left:
            return m_singleUseClicks.left;
        case ImGuiMouseButton_Right:
            return m_singleUseClicks.right;
        case ImGuiMouseButton_Middle:
            return m_singleUseClicks.middle;
        default:
            return false;
        }
    }

    /**
     * @brief Consume the click for the given frame
     */
    void consumeSingleUseClick(ImGuiMouseButton button)
    {
        switch (button) {
        case ImGuiMouseButton_Left:
            m_singleUseClicks.left = false;
            return;

        case ImGuiMouseButton_Right:
            m_singleUseClicks.right = false;
            return;

        case ImGuiMouseButton_Middle:
            m_singleUseClicks.middle = false;
            return;

        default:
            return;
        }
    }

    /**
     * @brief Get editor's name
     * @return Const reference to editor's name
     */
    const std::string& getName() { return m_name; }

    /**
     * @brief Get editor's position
     * @return Const reference to editor's position in screen coordinates
     */
    const ImVec2& getPos() { return m_context.origin(); }

    /**
     * @brief Get editor's grid scroll
     * @details Scroll is the offset from the origin of the grid, changes while navigating the grid.
     * @return Const reference to editor's grid scroll
     */
    const ImVec2& getScroll() { return m_context.scroll(); }

    /**
     * @brief Sets the context's scroll to a user specified value
     */
    void setScroll(ImVec2 scroll) { m_context.setScroll(scroll); }

    /**
     * @brief Sets the context's scale to a user specified value
     */
    void setScale(float scale) { m_context.setScale(scale); }

    /**
     * @brief Get the scale adjusted screen space mouse delta, needed for dragging
     *
     * @return scale adjusted mouse delta.
     */
    ImVec2 getScreenSpaceDelta() { return m_context.getScreenDelta(); }

    /**
     * @brief Get editor's list of nodes
     * @return Const reference to editor's internal nodes list
     */
    std::unordered_map<NodeUID, std::unique_ptr<BaseNode>, NodeUID::Hash>&
    getNodes()
    {
        return m_nodes;
    }

    /**
     * @brief Get nodes count
     * @return Number of nodes present in the editor
     */
    uint32_t getNodesCount() { return (uint32_t)m_nodes.size(); }

    /**
     * @brief Get editor's list of links
     * @return Const reference to editor's internal links list
     */
    const std::vector<std::unique_ptr<Link>>& getLinks() { return m_links; }

    /**
     * @brief Get zooming viewport
     * @return Const reference to editor's internal viewport for zoom support
     */
    ContainedContext& getGrid() { return m_context; }

    /**
     * @brief Get dragging status
     * @return [TRUE] if a Node is being dragged around the grid
     */
    [[nodiscard]] bool isNodeDragged() const { return m_draggingNode; }

    /**
     * @brief Get current style
     * @return Reference to style variables
     */
    InfStyler& getStyle() { return m_style; }

    /**
     * @brief Set editor's size
     * @param size Editor's size. Set to (0, 0) to auto-fit.
     */
    void setSize(const ImVec2& size) { m_context.config().size = size; }

    /**
     * @brief Set dragging status
     * @param state New dragging state
     *
     * The new state will only be updated one at the start of each frame.
     */
    void draggingNode(bool state) { m_draggingNodeNext = state; }

    /**
     * @brief Set what node is being hovered
     * @param hovering Pointer to the hovered node
     */
    void hoveredNode(BaseNode* hovering) { m_hoveredNode = hovering; }

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
    std::string m_name;
    ContainedContext m_context;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } m_singleUseClicks;

    std::unordered_map<NodeUID, std::unique_ptr<BaseNode>, NodeUID::Hash>
        m_nodes;
    std::vector<std::unique_ptr<Link>> m_links;

    std::function<void(Pin* dragged)> m_droppedLinkPopUp;
    ImGuiKey m_droppedLinkPupUpComboKey = ImGuiKey_None;
    Pin* m_droppedLinkLeft = nullptr;
    std::function<void(BaseNode* node)> m_rightClickPopUp;
    std::function<void(const ImVec2&)> m_dragdropTarget;
    BaseNode* m_hoveredNodeAux = nullptr;

    BaseNode* m_hoveredNode = nullptr;
    bool m_draggingNode = false;
    bool m_draggingNodeNext = false;
    Pin* m_hovering = nullptr;
    Pin* m_dragOut = nullptr;

    InfStyler m_style;

    struct SelectionBox {
        bool active = false;
        ImVec2 selectionStart;
        ImU32 selectionColor = IM_COL32(100, 150, 255, 50);
        ImU32 borderColor = IM_COL32(100, 150, 255, 200);
    };

    SelectionBox m_box;

    bool m_draggingCanvas = false;
};

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

/**
 * @brief Parent class for custom nodes
 * @details Main class from which custom nodes can be created. All interactions with the main grid are handled internally.
 */
class BaseNode {
public:
    explicit BaseNode(ImNodeFlow& flow, NodeUID uid)
        : m_flow(flow)
        , m_uid(uid)
    {
    }

    ~BaseNode();

    /**
     * @brief Main loop of the node
     * @details Updates position, hovering and selected status, and renders the node. Must be called each frame.
     */
    void update(ImNodeFlow& ctx);

    /**
     * @brief Content of the node
     * @details Function to be implemented by derived custom nodes.
     *	    Must contain the body of the node. If left empty the node will only have input and output pins.
     */
    void draw();

    /**
     * @brief Registers a unique observer for the node's draw call
     * @param func lambda function called when this Node's draw() method
     * is invoked
     */
    void on_drawHook(std::function<void()> func = []() -> void { });

    /**
     * @brief Registers a unique observer for the node's destruction event
     * @param func lambda function called when this Node is destroyed
     */
    void on_destroyHook(std::function<void()> func = []() -> void { });

    /**
     * @brief Add an Input to the node
     * @details Will add an Input pin to the node, constructed according to
     * specified InPinInfo
     * @tparam T Type of the data the pin will handle
     * @param pin_info Info passed along to the InPin for construction
     * @return Reference to the newly added pin.
     */
    template <typename T>
    Pin& addIN(InPinInfo<T> pin_info);

    /**
     * @brief Add an Input to the node
     * @details Will add an Input pin to the node, constructed according to
     * specified InPinInfo
     * @tparam T Type of the data the pin will handle
     * @param uid Creates an input pin with the specified uid, or replaces
     * an existing input pin with the same uid
     * @param pin_info Info passed along to the InPin for construction
     * @return Reference to the newly added pin.
     */
    template <typename T>
    Pin& addIN(const PinUID& uid, InPinInfo<T> pin_info);

    /**
     * @brief Remove input pin
     * @param uid Unique identifier of the pin
     */
    void dropIN(const PinUID& uid);

    /**
     * @brief Remove all input pin's with the associated name
     * @param name Pin's name filter
     */
    void dropIN(const char* name);

    /**
     * @brief Show a temporary input pin
     * @details Will show an input pin with the given name.
     *	    The pin is created the first time showIN is called and kept alive as long as showIN is called each frame.
     *	    In this case the name of the pin will also be its UID.
     *	    The UID must be unique only in the context of the current node's inputs.
     * @tparam T Type of the data the pin will handle
     * @param pin_info Info passed along to the InPin for construction
     * @return Const reference to the value of the connected link for the current frame of defaultValue
     */
    template <typename T>
    const T& showIN(InPinInfo<T> pin_info);

    /**
     * @brief Show a temporary input pin
     *
     * @details Will show an input pin with the given name and UID. The
     * pin is created the first time showIN_uid is called and kept alive
     * as long as showIN_uid is called each frame. The UID must be unique
     * only in the context of the current node's inputs.
     *
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info Info passed along to the InPin for construction
     * @return Const reference to the value of the connected link for the current frame of defaultValue
     */
    template <typename T>
    const T& showIN(const PinUID& uid, InPinInfo<T> pin_info);

    /**
     * @brief Add an Output to the node
     * @details Must be called in the node constructor. WIll add an Output pin to the node with the given name and data type.
     *	    In this case the name of the pin will also be its UID.
     *	    The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param pin_info Info passed along to the OutPin for construction
     * @return Reference to the newly added pin. Do not save this
     * reference. If you need to consult this specific pin, save it's
     * PinUID by calling getUid() and use that
     */
    template <typename T>
    Pin& addOUT(OutPinInfo<T> pin_info);

    /**
     * @brief Add an Output to the node
     * @details Must be called in the node constructor. WIll add an Output pin to the node with the given name and data type.
     *	    The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info Info passed along to the OutPin for construction
     * @return Reference to the newly added pin. Do not save this
     * reference. If you need to consult this specific pin, save it's
     * PinUID by calling getUid() and use that
     */
    template <typename T>
    Pin& addOUT(const PinUID& uid, OutPinInfo<T> pin_info);

    /**
     * @brief Remove output pin
     * @param uid Unique identifier of the pin
     */
    void dropOUT(const PinUID& uid);

    /**
     * @brief Remove all output pins with a matching name
     * @param name The name to filter for
     */
    void dropOUT(const char* name);

    /**
     * @brief Show a temporary output pin
     * @details Will show an output pin with the given name.
     *	    The pin is created the first time showOUT is called and kept alive as long as showOUT is called each frame.
     *	    In this case the name of the pin will also be its UID.
     *	    The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param pin_info Info passed along to the OutPin for construction
     */
    template <typename T>
    void showOUT(OutPinInfo<T> pin_info);

    /**
     * @brief Show a temporary output pin
     * @details Will show an output pin with the given name.
     *	    The pin is created the first time showOUT_uid is called and kept alive as long as showOUT_uid is called each frame.
     *	    The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param pin_info Info passed along to the OutPin for construction
     */
    template <typename T>
    void showOUT(const PinUID& uid, OutPinInfo<T> pin_info);

    /**
     * @brief Get Input value from an InPin
     * @details Get a reference to the value of an input pin, the value is stored in the output pin at the other end of the link.
     * @tparam T Data type
     * @param uid Unique identifier of the pin
     * @return Const reference to the value
     */
    template <typename T>
    const T& getInVal(const PinUID& uid);

    /**
     * @brief Get Input value from the first InPin with the specified name
     * @details Get a reference to the value of an input pin, the value is stored in the output pin at the other end of the link.
     * @tparam T Data type
     * @param name Name of the pin
     * @return Const reference to the value
     */
    template <typename T>
    const T& getInVal(const char* name);

    /**
     * @brief Get generic reference to input pin
     * @param uid Unique identifier of the pin
     * @return Reference to the pin
     */
    Pin& inPin(const PinUID& uid);

    /**
     * @brief Get generic reference to input pin with the specified name
     * @param uid Unique identifier of the pin
     * @return Reference to the pin
     */
    Pin& inPin(const char* name);

    /**
     * @brief Get generic reference to output pin
     * @param uid Unique identifier of the pin
     * @return Reference to the pin
     */
    Pin& outPin(const PinUID& uid);

    /**
     * @brief Get generic reference to output pin
     * @param uid Unique identifier of the pin
     * @return Reference to the pin
     */
    Pin& outPin(const char* uid);

    /**
     * @brief Get internal input pins list
     * @return Const reference to node's internal list
     */
    std::vector<std::unique_ptr<Pin>>& getIns() { return m_ins; }

    /**
     * @brief Get internal output pins list
     * @return Const reference to node's internal list
     */
    std::vector<std::unique_ptr<Pin>>& getOuts() { return m_outs; }

    /**
     * @brief Get internal dynamic input pins list
     * @return Const reference to node's internal list
     */
    std::vector<DynamicPin>& getDynamicIns() { return m_dynamicIns; }

    /**
     * @brief Get internal dynamic output pins list
     * @return Const reference to node's internal list
     */
    std::vector<DynamicPin>& getDynamicOuts() { return m_dynamicOuts; }

    /**
     * @brief Delete itself
     */
    void destroy()
    {
        if (m_destroyHook) {
            m_destroyHook();
        }
        m_destroyed = true;
    }

    /*
     * @brief Get if node must be deleted
     */
    [[nodiscard]] bool toDestroy() const { return m_destroyed; }

    /**
     * @brief Get node's UID
     * @return Node's unique identifier
     */
    [[nodiscard]] NodeUID getUID() const { return m_uid; }

    /**
     * @brief Get node name
     * @return Const reference to the node's name
     */
    [[nodiscard]] const std::string& getName() const { return m_title; }

    /**
     * @brief Get node size
     * @return Const reference to the node's size
     */
    const ImVec2& getSize() { return m_size; }

    /**
     * @brief Get node size
     * @return Const reference to the node's size
     */
    const ImVec2& getFullSize() { return m_fullSize; }

    /**
     * @brief Get node position
     * @return Const reference to the node's position
     */
    const ImVec2& getPos() { return m_pos; }

    /**
     * @brief Get node position
     * @return Const reference to the node's position
     */
    ImVec2 getTLPos()
    {
        return m_pos - ImVec2 { m_style.padding.x, m_style.padding.y };
    }

    /**
     * @brief Get node's style
     * @return Shared pointer to the node's style
     */
    const NodeStyle& getStyle() { return m_style; }

    /**
     * @brief Get selected status
     * @return [TRUE] if the node is selected
     */
    [[nodiscard]] bool isSelected() const
    {
        return m_selected || m_provisionallySelected;
    }

    /**
     * @brief Get dragged status
     * @return [TRUE] if the node is being dragged
     */
    [[nodiscard]] bool isDragged() const { return m_dragged; }

    /**
     * @brief Get Indestructable status
     * @return [TRUE] if the node is Indestructable
     */
    [[nodiscard]] bool isIndestructable() const { return m_indestructable; }

    /**
     * @brief Set node's uid
     * @param uid Node's unique identifier
     */
    BaseNode* setUID(NodeUID uid)
    {
        m_uid = uid;
        return this;
    }

    /**
     * @brief Set node's name
     * @param name New title
     */
    BaseNode* setTitle(const std::string& title)
    {
        m_title = title;
        return this;
    }

    /**
     * @brief Set node's position
     * @param pos Position in grid coordinates
     */
    BaseNode* setPos(const ImVec2& pos)
    {
        m_pos = pos;
        m_posTarget = pos;
        return this;
    }

    /**
     * @brief Set node's style
     * @param style New style
     */
    BaseNode* setStyle(NodeStyle style)
    {
        m_style = std::move(style);
        return this;
    }

    /**
     * @brief Set node's style
     * @param style New style
     */
    BaseNode* setIndestructable(bool indestruct)
    {
        m_indestructable = indestruct;
        return this;
    }

    /**
     * @brief Set selected status
     * @param state New selected state
     *
     * Status only updates when updatePublicStatus() is called
     */
    BaseNode* setSelected(bool state)
    {
        m_selectedNext = state;
        return this;
    }

    /**
     * @brief Set provisionally selected status
     * @param state New provisionally selected state
     *
     * Status only updates when updatePublicStatus() is called
     */
    BaseNode* setProvisionallySelected(bool state)
    {
        m_provisionallySelectedNext = state;
        return this;
    }

    /**
     * @brief Update the isSelected status of the node
     */
    void updatePublicStatus()
    {
        m_selected = m_selectedNext;
        m_provisionallySelected = m_provisionallySelectedNext;
        m_provisionallySelectedNext = false;
    }

    /**
     * @brief Returns whether or not the house was hovering in the last
     * update
     */
    bool isHovered() const { return m_is_hovered; }

    /**
     * @brief Have every pin calculate their next output value
     */
    void calculate();

    /**
     * @brief Have every pin commit their calculated values as their new
     * output value
     */
    void prime();

private:
    std::reference_wrapper<ImNodeFlow> m_flow;

    NodeUID m_uid;
    std::string m_title;
    ImVec2 m_pos;
    ImVec2 m_posTarget;
    ImVec2 m_size;
    ImVec2 m_fullSize;
    NodeStyle m_style = NodeStyles::default_style;

    bool m_is_hovered = false;
    bool m_selected = false;
    bool m_selectedNext = false;
    bool m_provisionallySelected = false;
    bool m_provisionallySelectedNext = false;
    bool m_dragged = false;
    bool m_destroyed = false;
    bool m_indestructable = false;

    std::function<void()> m_drawHook = []() -> void { };
    std::function<void()> m_destroyHook = []() -> void { };

    std::vector<std::unique_ptr<Pin>> m_ins;
    std::vector<DynamicPin> m_dynamicIns;
    std::vector<std::unique_ptr<Pin>> m_outs;
    std::vector<DynamicPin> m_dynamicOuts;
};

// -----------------------------------------------------------------------------------------------------------------
// PINS

/**
 * @brief Generic base class for pins
 */
class Pin {
public:
    /**
     * @brief Generic pin constructor
     * @param name Name of the pin
     * @param kind Specifies Input or Output
     * @param parent Pointer to the Node containing the pin
     * @param inf Pointer to the Grid Handler the pin is in (same as parent)
     * @param style Style of the pin
     */
    explicit Pin(
        ImNodeFlow& flow,
        BaseNode& parent,
        PinUID uid,
        BasePinInfo info)
        : m_flow(flow)
        , m_parent(parent)
        , m_uid(uid)
        , m_name(std::move(info.name))
        , m_kind(info.kind)
        , m_style(std::move(info.style))
        , m_strat(info.strat)
    {
    }

    virtual ~Pin()
    {
        // Make sure we disconnect from every connected link and mark
        // them for disconnection
        this->deleteAllLinks();
    }

    Pin(const Pin&) = delete; // no copy-ctor
    Pin(Pin&&) = delete; // no move-ctor
    Pin& operator=(const Pin&) = delete; // no copy operator
    Pin& operator=(Pin&&) = delete; // no move operator

    /**
     * @brief Safely casts a base Pin into the underlying InPin, if
     * possible. Crashes the program otherwise.
     *
     * @return Reference to the underlying InPin<T> if possible. Crash
     * otherwise.
     */
    template <typename T>
    InPin<T>& toIn();

    /**
     * @brief Safely casts a base Pin into the underlying InPin, if
     * possible. Crashes the program otherwise.
     *
     * @return Reference to the underlying InPin<T> if possible. Crash
     * otherwise.
     */
    template <typename T>
    InPin<T> const& toIn() const;

    /**
     * @brief Safely casts a base Pin into the underlying OutPin, if
     * possible. Crashes the program otherwise.
     *
     * @return Reference to the underlying OutPin<T> if possible. Crash
     * otherwise.
     */
    template <typename T>
    OutPin<T>& toOut();

    /**
     * @brief Safely casts a base Pin into the underlying OutPin, if
     * possible. Crashes the program otherwise.
     *
     * @return Reference to the underlying OutPin<T> if possible. Crash
     * otherwise.
     */
    template <typename T>
    OutPin<T> const& toOut() const;

    /**
     * @brief Main loop of the pin
     * @details Updates position, hovering and dragging status, and renders the pin. Must be called each frame.
     */
    void update();

    /**
     * @brief Draw default pin's socket
     */
    void drawSocket();

    /**
     * @brief Draw default pin's decoration (border, bg, and hover overlay)
     */
    void drawDecoration();

    /**
     * @brief Used by output pins to calculate their values
     */
    virtual void resolve() { }

    /**
     * @brief Custom render function to override Pin appearance
     * @param r Function or lambda expression with new ImGui rendering
     */
    Pin* renderer(std::function<void(Pin* p)> r)
    {
        m_renderer = std::move(r);
        return this;
    }

    /**
     * @brief Creates a new link between this pin and another pin
     */
    void linkTo(Pin& other);

    /**
     * @brief Handles the request for when a link is trying to connect
     * @param link Pointer to the link making the request
     */
    void addLink(Link* link);

    /**
     * @brief Delete all link reference
     */
    void deleteAllLinks();

    /**
     * @brief Delete a single link reference
     */
    virtual void removeLink(Link* link) { };

    /**
     * @brief Get connected status
     * @return [TRUE] if the pin is connected
     */
    bool isConnected() const { return !m_links.empty(); }

    /**
     * @brief Get all the links connected to this pin
     * @return A vector to an array of pointers to the connected links
     */
    std::vector<const Link*> getLinks() const
    {
        // Return by copy
        return std::vector<const Link*> { m_links.begin(), m_links.end() };
    }

    /**
     * @brief Get pin's UID
     * @return Unique identifier of the pin
     */
    [[nodiscard]] PinUID getUid() const { return m_uid; }

    /**
     * @brief Get pin's name
     * @return Const reference to pin's name
     */
    const std::string& getName() { return m_name; }

    /**
     * @brief Get pin's position
     * @return Const reference to pin's position in grid coordinates
     */
    [[nodiscard]] const ImVec2& getPos() { return m_pos; }

    /**
     * @brief Get pin's hit-box size
     * @return Const reference to pin's hit-box size
     */
    [[nodiscard]] const ImVec2& getSize() { return m_size; }

    /**
     * @brief Get pin's parent node
     * @return Generic type pointer to pin's parent node. (Node that contains it)
     */
    BaseNode& getParent() { return m_parent; }

    /**
     * @brief Get pin's type
     * @return The pin type. Either Input or Output
     */
    PinType getType() { return m_kind; }

    /**
     * @brief Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] virtual const uint64_t getTypeID() const = 0;

    /**
     * @brief Get pin's style
     * @return Smart pointer to pin's style
     */
    PinStyle& getStyle() { return m_style; }

    /**
     * @brief Specify if connections from an output on the same node are allowed
     * @param state New state of the flag
     */
    [[nodiscard]] bool allowsSameNodeConnections() const
    {
        return m_allowSelfConnection;
    }

    /**
     * @brief Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    virtual ImVec2 pinPoint() = 0;

    /**
     * @brief Calculate pin's width pre-rendering
     * @return The with of the pin once it will be rendered
     */
    float calcWidth() { return ImGui::CalcTextSize(m_name.c_str()).x; }

    /**
     * @brief Set pin's position
     * @param pos Position in screen coordinates
     */
    void setPos(ImVec2 pos) { m_pos = pos; }

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
    {
        m_socketHitboxEnabled = enabled;
    }

    /**
     * @brief Returns whether or not the mouse was hovering in the last
     * update
     */
    bool isHovered() const { return m_is_hovered; }

    /**
     * @brief Returns whether or not the mouse was clicked while hovering
     * in the last update
     */
    bool wasClicked() const { return m_was_clicked; }

    /**
     * @brief When the context calls this method, the pins should calculate
     * their next value, but not apply it. This allows every pin, during
     * the calculation step, to query what every other pin's output value
     * was in the closing iteration step.
     */
    virtual void calculate() = 0;

    /**
     * @brief When the context calls this method, the pins should update
     * their outputs to the computed value in calculate().
     *
     * During this step, pin's are no longer allowed to query any other
     * part of the flow graph, since their state is undefined.
     */
    virtual void prime() = 0;

protected:
    std::reference_wrapper<ImNodeFlow> m_flow;
    std::reference_wrapper<BaseNode> m_parent;

    PinUID m_uid;

    std::string m_name;
    PinType m_kind;
    PinStyle m_style;
    PinConnStrat m_strat;
    bool m_allowSelfConnection = false;
    bool m_socketHitboxEnabled = true;

    ImVec2 m_pos = ImVec2(0.F, 0.F);
    ImVec2 m_size = ImVec2(0.F, 0.F);

    bool m_is_hovered = false;
    bool m_was_clicked = false;

    std::function<void(Pin* p)> m_renderer;

    std::vector<Link*> m_links { };
};

/**
 * @brief Input specific pin
 * @details Derived from the generic class Pin. The input pin owns the link pointer.
 * @tparam T Data type handled by the pin
 */
template <class T>
class InPin : public Pin {
public:
    /**
     * @brief Input pin constructor
     * @param flow Reference to the ImNodeFlow context
     * @param parent Reference to the Node containing the pin
     * @param uid UID for the current pin
     * @param info User provided customization options
     */
    explicit InPin(
        ImNodeFlow& flow,
        BaseNode& parent,
        PinUID uid,
        InPinInfo<T> info)
        : Pin(flow,
              parent,
              uid,
              BasePinInfo {
                  .name = std::move(info.name),
                  .kind = PinType::PinType_Input,
                  .allow_same_node_connections = info.allow_same_node_connections,
                  .style = info.style,
                  .strat = info.strat,
              })
        , m_emptyVal(info.default_value)
        , m_val(info.default_value)
    {
        if (info.accumulate_op) {
            m_accumulatorOp = std::move(info.accumulate_op);
        } else {
            switch (info.strat) {
            case PinConnStrat::PinConnStrat_AllowOnlySingleConnection:
            case PinConnStrat::PinConnStrat_AllowMultiplePreferOldest:
                m_accumulatorOp =
                    [](bool first_pin,
                        T init,
                        T const& incoming_value) -> T {
                    return incoming_value;
                };
                break;

            case PinConnStrat::PinConnStrat_AllowMultiplePreferNewest:
                m_accumulatorOp =
                    [](bool first_pin,
                        T init,
                        T const& incoming_value) -> T {
                    return first_pin ? incoming_value : init;
                };
            }
        }
    }

    /**
     * @brief Marks all edges connected to this InPoint for deletion
     */
    ~InPin() override;

    /**
     * @brief Removes any reference to this link, if it's held
     */
    void removeLink(Link* link) override;

    /**
     * @brief Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] const uint64_t getTypeID() const override
    {
        return detail::get_type_id<T>();
    };

    /**
     * @brief Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    ImVec2 pinPoint() override
    {
        return m_pos + ImVec2(-getStyle().extra.socket_padding, m_size.y / 2);
    }

    void calculate() override;

    void prime() override;

    /**
     * @brief Get value carried by the connected link
     * @return Reference to the value of the connected OutPin. Or the default value if not connected
     */
    const T& val();

private:
    T m_emptyVal;
    T m_val;
    T m_valNext;
    std::function<T(bool first_pin, T init, T const& incoming_value)>
        m_accumulatorOp;
};

/**
 * @brief Output specific pin
 * @details Derived from the generic class Pin. The output pin handles the logic.
 * @tparam T Data type handled by the pin
 */
template <class T>
class OutPin : public Pin {
public:
    /**
     * @brief Output pin constructor
     * @param flow Reference to the ImNodeFlow context
     * @param parent Reference to the Node containing the pin
     * @param uid UID for the current pin
     * @param info User provided customization options
     *
     * Note: Please make sure to not depend on the value of other output
     * pins in your behaviour lambda, as that will very likely result in
     * a non-deterministic update step.
     */
    explicit OutPin(
        ImNodeFlow& flow,
        BaseNode& parent,
        PinUID uid,
        OutPinInfo<T> info)
        : Pin(flow,
              parent,
              uid,
              BasePinInfo {
                  .name = std::move(info.name),
                  .kind = PinType_Output,
                  .allow_same_node_connections = info.allow_same_node_connections,
                  .style = info.style,
                  .strat = info.strat,
              })
        , m_behaviour(std::move(info.behaviour))
    {
    }

    /**
     * @brief When parent gets deleted, remove the links
     */
    ~OutPin() override;

    /**
     * @brief Removes any reference to this link, if it's held
     */
    void removeLink(Link* link) override;

    /**
     * @brief Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    ImVec2 pinPoint() override
    {
        return m_pos + ImVec2(m_size.x + m_style.extra.socket_padding, m_size.y / 2);
    }

    /**
     * @brief Updates the behaviour function for this output pin
     * @param new_behaviour The behaviour which will replace the existing
     * one
     */
    void setBehaviour(std::function<T()> new_behaviour);

    void calculate() override;

    void prime() override;

    /**
     * @brief Get output value
     * @return Const reference to the internal value of the pin
     */
    const T& val();

    /**
     * @brief Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] const uint64_t getTypeID() const override
    {
        return detail::get_type_id<T>();
    };

private:
    std::function<T()> m_behaviour;
    T m_val;
    T m_valNext;
};

} // namespace ImFlow

#include "../src/ImNodeFlow.inl"

#endif
