#ifndef IM_NODE_FLOW
#define IM_NODE_FLOW
#pragma once

#include "../src/context_wrapper.h"
#include "../src/imgui_bezier_math.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ImFlow {

namespace detail {

    /**
     * @brief <BR>Draw a sensible bezier between two points
     * @param p1 Starting point
     * @param p2 Ending point
     * @param color Color of the curve
     * @param thickness Thickness of the curve
     */
    inline static void
    smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness);

    /**
     * @brief <BR>Collider checker for smart_bezier
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
     * This allows connection filters to work without RTTI
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
class IFilter;
class ConnectionFilter;

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
    PinStyle(
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

public:
    /// @brief <BR>Default cyan style
    static std::shared_ptr<PinStyle> cyan()
    {
        return std::make_shared<PinStyle>(PinStyle(
            IM_COL32(87, 155, 185, 255), 0, 4.f, 4.67f, 3.7f, 1.f));
    }
    /// @brief <BR>Default green style
    static std::shared_ptr<PinStyle> green()
    {
        return std::make_shared<PinStyle>(PinStyle(
            IM_COL32(90, 191, 93, 255), 4, 4.f, 4.67f, 4.2f, 1.3f));
    }
    /// @brief <BR>Default blue style
    static std::shared_ptr<PinStyle> blue()
    {
        return std::make_shared<PinStyle>(PinStyle(
            IM_COL32(90, 117, 191, 255), 0, 4.f, 4.67f, 3.7f, 1.f));
    }
    /// @brief <BR>Default brown style
    static std::shared_ptr<PinStyle> brown()
    {
        return std::make_shared<PinStyle>(PinStyle(
            IM_COL32(191, 134, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f));
    }
    /// @brief <BR>Default red style
    static std::shared_ptr<PinStyle> red()
    {
        return std::make_shared<PinStyle>(PinStyle(
            IM_COL32(191, 90, 90, 255), 0, 4.f, 4.67f, 3.7f, 1.f));
    }
    /// @brief <BR>Default white style
    static std::shared_ptr<PinStyle> white()
    {
        return std::make_shared<PinStyle>(PinStyle(
            IM_COL32(255, 255, 255, 255), 5, 4.f, 4.67f, 4.2f, 1.f));
    }
};

// -----------------------------------------------------------------------------------------------------------------
// NODE'S PROPERTIES

typedef uintptr_t NodeUID;

/**
 * @brief Defines the visual appearance of a node
 */
class NodeStyle {
public:
    NodeStyle(ImU32 header_bg, ImColor header_title_color, float radius)
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

public:
    /// @brief <BR>Default cyan style
    static std::shared_ptr<NodeStyle> cyan()
    {
        return std::make_shared<NodeStyle>(
            IM_COL32(71, 142, 173, 255),
            ImColor(233, 241, 244, 255),
            6.5f);
    }
    /// @brief <BR>Default green style
    static std::shared_ptr<NodeStyle> green()
    {
        return std::make_shared<NodeStyle>(
            IM_COL32(90, 191, 93, 255),
            ImColor(233, 241, 244, 255),
            3.5f);
    }
    /// @brief <BR>Default red style
    static std::shared_ptr<NodeStyle> red()
    {
        return std::make_shared<NodeStyle>(
            IM_COL32(191, 90, 90, 255),
            ImColor(233, 241, 244, 255),
            11.f);
    }
    /// @brief <BR>Default brown style
    static std::shared_ptr<NodeStyle> brown()
    {
        return std::make_shared<NodeStyle>(
            IM_COL32(191, 134, 90, 255),
            ImColor(233, 241, 244, 255),
            6.5f);
    }
};

// -----------------------------------------------------------------------------------------------------------------
// LINK

/**
 * @brief Link between two Pins of two different Nodes
 */
class Link {
public:
    /**
     * @brief <BR>Construct a link
     * @param out_pin Pointer to the output Pin of the Link
     * @param in_pin Pointer to the input Pin of the Link
     * @param inf Pointer to the Handler that contains the Link
     */
    explicit Link(Pin& out_pin, Pin& in_pin)
        : m_out_pin(&out_pin)
        , m_in_pin(&in_pin)
    {
    }

    /**
     * @brief Looping function to update the Link
     * @details Draws the Link and updates Hovering and Selected status.
     */
    void update(ImNodeFlow& ctx);

    /**
     * @brief <BR>Get output pin of the link
     * @return Pointer to the Pin
     */
    [[nodiscard]] Pin* outPin() const { return m_out_pin; }

    /**
     * @brief <BR>Get input pin of the link
     * @return Pointer to the Pin
     */
    [[nodiscard]] Pin* inPin() const { return m_in_pin; }

    /**
     * @brief <BR>Get hovering status
     * @return [TRUE] If the link is hovered in the current frame
     */
    [[nodiscard]] bool isHovered() const { return m_hovered; }

    /**
     * @brief <BR>Get selected status
     * @return [TRUE] If the link is selected in the current frame
     */
    [[nodiscard]] bool isSelected() const { return m_selected; }

    /**
     * @brief Marks this link for deletion
     */
    void markForDeletion() { m_marked_for_deletion = true; }

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
     * @brief <BR>Instantiate a new editor with default name.
     * <BR> Editor name will be "FlowGrid + the number of editors"
     */
    ImNodeFlow()
        : ImNodeFlow("FlowGrid" + std::to_string(m_instances))
    {
    }

    /**
     * @brief <BR>Instantiate a new editor with given name
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
     * @brief <BR>Handler loop
     * @details Main update function. Refreshes all the logic and draws everything. Must be called every frame.
     */
    void update();

    /**
     * @brief <BR>Add a node to the grid
     * @tparam T Derived class of <BaseNode> to be added
     * @tparam Params types of optional args to forward to derived class ctor
     * @param pos Position of the Node in grid coordinates
     * @param args Optional arguments to be forwarded to derived class ctor
     * @return Shared pointer of the pushed type to the newly added node
     *
     * Inheritance is checked at compile time, \<T> MUST be derived from BaseNode.
     */
    template <typename T, typename... Params>
    std::shared_ptr<T> addNode(const ImVec2& pos, Params&&... args);

private:
    /**
     * @brief <BR>Helper struct for creating a node struct from a lambda
     * @sa addLambdaNode which wraps creating one of these
     * @tparam L the type of the lambda
     * @tparam B always BaseNode, tparam because BaseNode is incomplete here
     */
    template <typename L, typename B = BaseNode>
    struct NodeWrapper : public B {
        L mLambda;
        NodeWrapper(L&& l)
            : BaseNode()
            , mLambda(std::forward<L>(l))
        {
        }
        void draw() { mLambda(this); }
    };

public:
    /**
     * @brief <BR>Add a node whos operation can be defined within a lambda.
     * @tparam L the type of the lambda
     * @param lambda the lambda that defines the nodes operation
     * @param pos the position at which to place the node
     */
    template <typename L>
    std::shared_ptr<NodeWrapper<L>>
    addLambdaNode(L&& lambda, const ImVec2& pos)
    {
        return addNode<NodeWrapper<L>>(pos, std::forward<L>(lambda));
    }

    /**
     * @brief <BR>Add a node to the grid
     * @tparam T Derived class of <BaseNode> to be added
     * @tparam Params types of optional args to forward to derived class ctor
     * @param pos Position of the Node in screen coordinates
     * @param args Optional arguments to be forwarded to derived class ctor
     * @return Shared pointer of the pushed type to the newly added node
     *
     * Inheritance is checked at compile time, \<T> MUST be derived from BaseNode.
     */
    template <typename T, typename... Params>
    std::shared_ptr<T> placeNodeAt(const ImVec2& pos, Params&&... args);

    /**
     * @brief <BR>Add a node to the grid using mouse position
     * @tparam T Derived class of <BaseNode> to be added
     * @tparam Params types of optional args to forward to derived class ctor
     * @param args Optional arguments to be forwarded to derived class ctor
     * @return Shared pointer of the pushed type to the newly added node
     *
     * Inheritance is checked at compile time, \<T> MUST be derived from BaseNode.
     */
    template <typename T, typename... Params>
    std::shared_ptr<T> placeNode(Params&&... args);

    /**
     * @brief Creates a link between two pins
     * @param source_pin Reference to the input pin
     * @param target_pint Reference to the output pin
     */
    void createLink(Pin& source_pin, Pin& target_pin);

    /**
     * @brief <BR>Pop-up when link is "dropped"
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
     * @brief <BR>Pop-up when right-clicking
     * @details Sets the content of a pop-up that can be displayed when right-clicking on the grid.
     * @param content Function or Lambda containing only the contents of the pop-up and the subsequent logic
     */
    void rightClickPopUpContent(std::function<void(BaseNode* node)> content)
    {
        m_rightClickPopUp = std::move(content);
    }

    /**
     * @brief <BR>Get mouse clicking status
     * @return [TRUE] if mouse is clicked and click hasn't been consumed
     */
    [[nodiscard]] bool getSingleUseClick() const
    {
        return m_singleUseClick;
    }

    /**
     * @brief <BR>Consume the click for the given frame
     */
    void consumeSingleUseClick() { m_singleUseClick = false; }

    /**
     * @brief <BR>Get editor's name
     * @return Const reference to editor's name
     */
    const std::string& getName() { return m_name; }

    /**
     * @brief <BR>Get editor's position
     * @return Const reference to editor's position in screen coordinates
     */
    const ImVec2& getPos() { return m_context.origin(); }

    /**
     * @brief <BR>Get editor's grid scroll
     * @details Scroll is the offset from the origin of the grid, changes while navigating the grid.
     * @return Const reference to editor's grid scroll
     */
    const ImVec2& getScroll() { return m_context.scroll(); }

    /**
     * @brief <BR>Get the scale adjusted screen space mouse delta, needed for dragging
     *
     * @return scale adjusted mouse delta.
     */
    ImVec2 getScreenSpaceDelta() { return m_context.getScreenDelta(); }

    /**
     * @brief <BR>Get editor's list of nodes
     * @return Const reference to editor's internal nodes list
     */
    std::unordered_map<NodeUID, std::shared_ptr<BaseNode>>& getNodes()
    {
        return m_nodes;
    }

    /**
     * @brief <BR>Get nodes count
     * @return Number of nodes present in the editor
     */
    uint32_t getNodesCount() { return (uint32_t)m_nodes.size(); }

    /**
     * @brief <BR>Get editor's list of links
     * @return Const reference to editor's internal links list
     */
    const std::vector<std::shared_ptr<Link>>& getLinks() { return m_links; }

    /**
     * @brief <BR>Get zooming viewport
     * @return Const reference to editor's internal viewport for zoom support
     */
    ContainedContext& getGrid() { return m_context; }

    /**
     * @brief <BR>Get dragging status
     * @return [TRUE] if a Node is being dragged around the grid
     */
    [[nodiscard]] bool isNodeDragged() const { return m_draggingNode; }

    /**
     * @brief <BR>Get current style
     * @return Reference to style variables
     */
    InfStyler& getStyle() { return m_style; }

    /**
     * @brief <BR>Set editor's size
     * @param size Editor's size. Set to (0, 0) to auto-fit.
     */
    void setSize(const ImVec2& size) { m_context.config().size = size; }

    /**
     * @brief <BR>Set dragging status
     * @param state New dragging state
     *
     * The new state will only be updated one at the start of each frame.
     */
    void draggingNode(bool state) { m_draggingNodeNext = state; }

    /**
     * @brief <BR>Set what node is being hovered
     * @param hovering Pointer to the hovered node
     */
    void hoveredNode(BaseNode* hovering) { m_hoveredNode = hovering; }

    /**
     * @brief <BR>Convert coordinates from screen to grid
     * @param p Point in screen coordinates to be converted
     * @return Point in grid's coordinates
     */
    ImVec2 screen2grid(const ImVec2& p);

    /**
     * @brief <BR>Convert coordinates from grid to screen
     * @param p Point in grid's coordinates to be converted
     * @return Point in screen coordinates
     */
    ImVec2 grid2screen(const ImVec2& p);

    /**
     * @brief <BR>Check if mouse is on selected node
     * @return [TRUE] if the mouse is hovering a selected node
     */
    bool mouse_on_selected_node();

    /**
     * @brief <BR>Check if mouse is on a free point on the grid
     * @return [TRUE] if the mouse is not hovering a node or a link
     */
    bool mouse_on_free_space();

private:
    std::string m_name;
    ContainedContext m_context;

    bool m_singleUseClick = false;

    std::unordered_map<NodeUID, std::shared_ptr<BaseNode>> m_nodes;
    std::vector<std::shared_ptr<Link>> m_links;

    std::function<void(Pin* dragged)> m_droppedLinkPopUp;
    ImGuiKey m_droppedLinkPupUpComboKey = ImGuiKey_None;
    Pin* m_droppedLinkLeft = nullptr;
    std::function<void(BaseNode* node)> m_rightClickPopUp;
    BaseNode* m_hoveredNodeAux = nullptr;

    BaseNode* m_hoveredNode = nullptr;
    bool m_draggingNode = false;
    bool m_draggingNodeNext = false;
    Pin* m_hovering = nullptr;
    Pin* m_dragOut = nullptr;

    InfStyler m_style;

    struct SelectionBox {
        bool active;
        ImVec2 selectionStart;
        ImU32 selectionColor = IM_COL32(100, 150, 255, 50);
        ImU32 borderColor = IM_COL32(100, 150, 255, 200);
    };

    SelectionBox m_box;
};

// -----------------------------------------------------------------------------------------------------------------
// BASE NODE

/**
 * @brief Parent class for custom nodes
 * @details Main class from which custom nodes can be created. All interactions with the main grid are handled internally.
 */
class BaseNode {
public:
    virtual ~BaseNode() = default;
    BaseNode() = default;

    /**
     * @brief <BR>Main loop of the node
     * @details Updates position, hovering and selected status, and renders the node. Must be called each frame.
     */
    void update(ImNodeFlow& ctx);

    /**
     * @brief <BR>Content of the node
     * @details Function to be implemented by derived custom nodes.
     *	    Must contain the body of the node. If left empty the node will only have input and output pins.
     */
    virtual void draw() { }

    /**
     * @brief <BR>Add an Input to the node
     * @details Will add an Input pin to the node with the given name and data type.
     *	    <BR> <BR> In this case the name of the pin will also be its UID.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's inputs.
     * @tparam T Type of the data the pin will handle
     * @param name Name of the pin
     * @param defReturn Default return value when the pin is not connected
     * @param filter Connection filter
     * @param style Style of the pin
     * @return Reference to the newly added pin. Do not save this
     * reference. If you need to consult this specific pin, save it's
     * PinUID by calling getUid() and use that
     */
    template <typename T>
    InPin<T>&
    addIN(const std::string& name,
        T defReturn,
        std::shared_ptr<IFilter> filter,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Add an Input to the node
     * @details Will add an Input pin to the node with the given name and data type.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's inputs.
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param name Name of the pin
     * @param defReturn Default return value when the pin is not connected
     * @param filter Connection filter
     * @param style Style of the pin
     * @return Reference to the newly added pin. Do not save this
     * reference. If you need to consult this specific pin, save it's
     * PinUID by calling getUid() and use that
     */
    template <typename T>
    InPin<T>& addIN_uid(
        const PinUID& uid,
        const std::string& name,
        T defReturn,
        std::shared_ptr<IFilter> filter,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Remove input pin
     * @param uid Unique identifier of the pin
     */
    void dropIN(const PinUID& uid);

    /**
     * @brief Remove all input pin's with the associated name
     * @param name Pin's name filter
     */
    void dropIN(const char* name);

    /**
     * @brief <BR>Show a temporary input pin
     * @details Will show an input pin with the given name.
     *	    The pin is created the first time showIN is called and kept alive as long as showIN is called each frame.
     *	    <BR> <BR> In this case the name of the pin will also be its UID.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's inputs.
     * @tparam T Type of the data the pin will handle
     * @param name Name of the pin
     * @param defReturn Default return value when the pin is not connected
     * @param filter Connection filter
     * @param style Style of the pin
     * @return Const reference to the value of the connected link for the current frame of defReturn
     */
    template <typename T>
    const T&
    showIN(const std::string& name,
        T defReturn,
        std::shared_ptr<IFilter> filter,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Show a temporary input pin
     * @details Will show an input pin with the given name and UID.
     *	    The pin is created the first time showIN_uid is called and kept alive as long as showIN_uid is called each frame.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's inputs.
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param name Name of the pin
     * @param defReturn Default return value when the pin is not connected
     * @param filter Connection filter
     * @param style Style of the pin
     * @return Const reference to the value of the connected link for the current frame of defReturn
     */
    template <typename T>
    const T& showIN_uid(
        const PinUID& uid,
        const std::string& name,
        T defReturn,
        std::shared_ptr<IFilter> filter,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Add an Output to the node
     * @details Must be called in the node constructor. WIll add an Output pin to the node with the given name and data type.
     *	    <BR> <BR> In this case the name of the pin will also be its UID.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param name Name of the pin
     * @param filter Connection filter
     * @param style Style of the pin
     * @return Reference to the newly added pin. Do not save this
     * reference. If you need to consult this specific pin, save it's
     * PinUID by calling getUid() and use that
     */
    template <typename T>
    [[nodiscard]] OutPin<T>&
    addOUT(const std::string& name,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Add an Output to the node
     * @details Must be called in the node constructor. WIll add an Output pin to the node with the given name and data type.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param name Name of the pin
     * @param filter Connection filter
     * @param style Style of the pin
     * @return Reference to the newly added pin. Do not save this
     * reference. If you need to consult this specific pin, save it's
     * PinUID by calling getUid() and use that
     */
    template <typename T>
    [[nodiscard]] OutPin<T>& addOUT_uid(
        const PinUID& uid,
        const std::string& name,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Remove output pin
     * @param uid Unique identifier of the pin
     */
    void dropOUT(const PinUID& uid);

    /**
     * @brief Remove all output pins with a matching name
     * @param name The name to filter for
     */
    void dropOUT(const char* name);

    /**
     * @brief <BR>Show a temporary output pin
     * @details Will show an output pin with the given name.
     *	    The pin is created the first time showOUT is called and kept alive as long as showOUT is called each frame.
     *	    <BR> <BR> In this case the name of the pin will also be its UID.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param name Name of the pin
     * @param behaviour Function or lambda expression used to calculate output value
     * @param filter Connection filter
     * @param style Style of the pin
     */
    template <typename T>
    void
    showOUT(const std::string& name,
        std::function<T()> behaviour,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Show a temporary output pin
     * @details Will show an output pin with the given name.
     *	    The pin is created the first time showOUT_uid is called and kept alive as long as showOUT_uid is called each frame.
     *	    <BR> <BR> The UID must be unique only in the context of the current node's outputs.
     * @tparam T Type of the data the pin will handle
     * @param uid Unique identifier of the pin
     * @param name Name of the pin
     * @param behaviour Function or lambda expression used to calculate output value
     * @param filter Connection filter
     * @param style Style of the pin
     */
    template <typename T>
    void showOUT_uid(
        const PinUID& uid,
        const std::string& name,
        std::function<T()> behaviour,
        std::shared_ptr<PinStyle> style = nullptr);

    /**
     * @brief <BR>Get Input value from an InPin
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
     * @brief <BR>Get generic reference to input pin
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
     * @brief <BR>Get generic reference to output pin
     * @param uid Unique identifier of the pin
     * @return Reference to the pin
     */
    Pin& outPin(const PinUID& uid);

    /**
     * @brief <BR>Get generic reference to output pin
     * @param uid Unique identifier of the pin
     * @return Reference to the pin
     */
    Pin& outPin(const char* uid);

    /**
     * @brief <BR>Get internal input pins list
     * @return Const reference to node's internal list
     */
    const std::vector<std::shared_ptr<Pin>>& getIns() { return m_ins; }

    /**
     * @brief <BR>Get internal output pins list
     * @return Const reference to node's internal list
     */
    const std::vector<std::shared_ptr<Pin>>& getOuts() { return m_outs; }

    /**
     * @brief <BR>Get internal dynamic input pins list
     * @return Const reference to node's internal list
     */
    const std::vector<std::pair<int, std::shared_ptr<Pin>>>& getDynamicIns()
    {
        return m_dynamicIns;
    }

    /**
     * @brief <BR>Get internal dynamic output pins list
     * @return Const reference to node's internal list
     */
    const std::vector<std::pair<int, std::shared_ptr<Pin>>>&
    getDynamicOuts()
    {
        return m_dynamicOuts;
    }

    /**
     * @brief <BR>Delete itself
     */
    void destroy() { m_destroyed = true; }

    /*
     * @brief <BR>Get if node must be deleted
     */
    [[nodiscard]] bool toDestroy() const { return m_destroyed; }

    /**
     * @brief <BR>Get node's UID
     * @return Node's unique identifier
     */
    [[nodiscard]] NodeUID getUID() const { return m_uid; }

    /**
     * @brief <BR>Get node name
     * @return Const reference to the node's name
     */
    const std::string& getName() { return m_title; }

    /**
     * @brief <BR>Get node size
     * @return Const reference to the node's size
     */
    const ImVec2& getSize() { return m_size; }

    /**
     * @brief <BR>Get node size
     * @return Const reference to the node's size
     */
    const ImVec2& getFullSize() { return m_fullSize; }

    /**
     * @brief <BR>Get node position
     * @return Const reference to the node's position
     */
    const ImVec2& getPos() { return m_pos; }

    /**
     * @brief <BR>Get node's style
     * @return Shared pointer to the node's style
     */
    const std::shared_ptr<NodeStyle>& getStyle() { return m_style; }

    /**
     * @brief <BR>Get selected status
     * @return [TRUE] if the node is selected
     */
    [[nodiscard]] bool isSelected() const
    {
        return m_selected || m_provisionallySelected;
    }

    /**
     * @brief <BR>Get dragged status
     * @return [TRUE] if the node is being dragged
     */
    [[nodiscard]] bool isDragged() const { return m_dragged; }

    /**
     * @brief <BR>Set node's uid
     * @param uid Node's unique identifier
     */
    BaseNode* setUID(NodeUID uid)
    {
        m_uid = uid;
        return this;
    }

    /**
     * @brief <BR>Set node's name
     * @param name New title
     */
    BaseNode* setTitle(const std::string& title)
    {
        m_title = title;
        return this;
    }

    /**
     * @brief <BR>Set node's position
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
    BaseNode* setStyle(std::shared_ptr<NodeStyle> style)
    {
        m_style = std::move(style);
        return this;
    }

    /**
     * @brief <BR>Set selected status
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
     * @brief <BR>Update the isSelected status of the node
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
    NodeUID m_uid = 0;
    std::string m_title;
    ImVec2 m_pos;
    ImVec2 m_posTarget;
    ImVec2 m_size;
    ImVec2 m_fullSize;
    std::shared_ptr<NodeStyle> m_style;
    bool m_is_hovered = false;
    bool m_selected = false;
    bool m_selectedNext = false;
    bool m_provisionallySelected = false;
    bool m_provisionallySelectedNext = false;
    bool m_dragged = false;
    bool m_destroyed = false;

    std::vector<std::shared_ptr<Pin>> m_ins;
    std::vector<std::pair<int, std::shared_ptr<Pin>>> m_dynamicIns;
    std::vector<std::shared_ptr<Pin>> m_outs;
    std::vector<std::pair<int, std::shared_ptr<Pin>>> m_dynamicOuts;
};

// -----------------------------------------------------------------------------------------------------------------
// PINS

/**
 * @brief Pins type identifier
 */
enum PinType { PinType_Input,
    PinType_Output };

/**
 * @brief Generic base class for pins
 */
class Pin {
public:
    /**
     * @brief <BR>Generic pin constructor
     * @param name Name of the pin
     * @param filter Connection filter
     * @param kind Specifies Input or Output
     * @param parent Pointer to the Node containing the pin
     * @param inf Pointer to the Grid Handler the pin is in (same as parent)
     * @param style Style of the pin
     */
    explicit Pin(
        PinUID uid,
        std::string name,
        std::shared_ptr<PinStyle> style,
        PinType kind,
        BaseNode* parent)
        : m_uid(uid)
        , m_name(std::move(name))
        , m_type(kind)
        , m_parent(parent)
        , m_style(std::move(style))
    {
        if (!m_style)
            m_style = PinStyle::cyan();
    }

    virtual ~Pin() = default;

    /**
     * @brief <BR>Main loop of the pin
     * @details Updates position, hovering and dragging status, and renders the pin. Must be called each frame.
     */
    void update(ImNodeFlow& ctx);

    /**
     * @brief <BR>Draw default pin's socket
     */
    void drawSocket();

    /**
     * @brief <BR>Draw default pin's decoration (border, bg, and hover overlay)
     */
    void drawDecoration();

    /**
     * @brief <BR>Used by output pins to calculate their values
     */
    virtual void resolve() { }

    /**
     * @brief <BR>Custom render function to override Pin appearance
     * @param r Function or lambda expression with new ImGui rendering
     */
    Pin* renderer(std::function<void(Pin* p)> r)
    {
        m_renderer = std::move(r);
        return this;
    }

    /**
     * @brief <BR>Create link between pins
     * @param other Pointer to the other pin
     */
    virtual bool canCreateLink(Pin& other) = 0;

    /**
     * @brief <BR>Set the reference to a link
     * @param link Smart pointer to the link
     */
    virtual void setLink(std::weak_ptr<Link> link) = 0;

    /**
     * @brief <BR>Delete link reference
     */
    virtual void deleteLink() = 0;

    /**
     * @brief <BR>Get connected status
     * @return [TRUE] if the pin is connected
     */
    virtual bool isConnected() = 0;

    /**
     * @brief <BR>Get pin's link
     * @return Weak_ptr reference to pin's link
     */
    virtual std::weak_ptr<Link> getLink() { return std::weak_ptr<Link> { }; }

    /**
     * @brief <BR>Get pin's UID
     * @return Unique identifier of the pin
     */
    [[nodiscard]] PinUID getUid() const { return m_uid; }

    /**
     * @brief <BR>Get pin's name
     * @return Const reference to pin's name
     */
    const std::string& getName() { return m_name; }

    /**
     * @brief <BR>Get pin's position
     * @return Const reference to pin's position in grid coordinates
     */
    [[nodiscard]] const ImVec2& getPos() { return m_pos; }

    /**
     * @brief <BR>Get pin's hit-box size
     * @return Const reference to pin's hit-box size
     */
    [[nodiscard]] const ImVec2& getSize() { return m_size; }

    /**
     * @brief <BR>Get pin's parent node
     * @return Generic type pointer to pin's parent node. (Node that contains it)
     */
    BaseNode* getParent() { return m_parent; }

    /**
     * @brief <BR>Get pin's type
     * @return The pin type. Either Input or Output
     */
    PinType getType() { return m_type; }

    /**
     * @brief <BR>Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] virtual const uint64_t getTypeID() const = 0;

    /**
     * @brief <BR>Get pin's style
     * @return Smart pointer to pin's style
     */
    std::shared_ptr<PinStyle>& getStyle() { return m_style; }

    /**
     * @brief <BR>Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    virtual ImVec2 pinPoint() = 0;

    /**
     * @brief <BR>Calculate pin's width pre-rendering
     * @return The with of the pin once it will be rendered
     */
    float calcWidth() { return ImGui::CalcTextSize(m_name.c_str()).x; }

    /**
     * @brief <BR>Set pin's position
     * @param pos Position in screen coordinates
     */
    void setPos(ImVec2 pos) { m_pos = pos; }

    /**
     * @brief Returns whether or not the house was hovering in the last
     * update
     */
    bool isHovered() const { return m_is_hovered; }

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
    PinUID m_uid;
    std::string m_name;
    ImVec2 m_pos = ImVec2(0.f, 0.f);
    ImVec2 m_size = ImVec2(0.f, 0.f);
    PinType m_type;
    BaseNode* m_parent = nullptr;
    std::shared_ptr<PinStyle> m_style;
    std::function<void(Pin* p)> m_renderer;
    bool m_is_hovered = false;
};

/**
 * @brief Common filter interface
 */
class IFilter {
public:
    virtual bool operator()(Pin const& out, Pin const& in) const = 0;
};

/**
 * @brief Collection of Pin's collection filters
 */
class ConnectionFilter {
public:
    struct None_t : public IFilter {
        bool operator()(Pin const& out, Pin const& in) const final
        {
            return true;
        }
    };

    struct SameType_t : public IFilter {
        bool operator()(Pin const& out, Pin const& in) const final
        {
            return out.getTypeID() == in.getTypeID();
        }
    };

    struct Numbers_t : public IFilter {
        bool operator()(Pin const& out, Pin const& in) const final
        {
            return out.getTypeID() == detail::get_type_id<double>() || out.getTypeID() == detail::get_type_id<float>() || out.getTypeID() == detail::get_type_id<int>();
        }
    };

    static std::shared_ptr<IFilter> None()
    {
        return std::make_shared<None_t>();
    }

    static std::shared_ptr<IFilter> SameType()
    {
        return std::make_shared<SameType_t>();
    }

    static std::shared_ptr<IFilter> Numbers()
    {
        return std::make_shared<Numbers_t>();
    }
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
     * @brief <BR>Input pin constructor
     * @param name Name of the pin
     * @param filter Connection filter
     * @param parent Pointer to the Node containing the pin
     * @param defReturn Default return value when the pin is not connected
     * @param inf Pointer to the Grid Handler the pin is in (same as parent)
     * @param style Style of the pin
     */
    explicit InPin(
        PinUID uid,
        const std::string& name,
        T defReturn,
        std::shared_ptr<IFilter> filter,
        std::shared_ptr<PinStyle> style,
        BaseNode* parent)
        : Pin(uid, name, style, PinType_Input, parent)
        , m_emptyVal(defReturn)
        , m_val(defReturn)
        , m_filter(std::move(filter))
    {
    }

    /**
     * @brief Marks all edges connected to this InPoint for deletion
     */
    ~InPin() override;

    /**
     * @brief <BR>Create link between pins
     * @param other Pointer to the other pin
     */
    bool canCreateLink(Pin& other) override;

    /**
     * @brief <BR>Add a connected link to the internal list
     * @param link Pointer to the link
     */
    void setLink(std::weak_ptr<Link> link) override;

    /**
     * @brief <BR>Delete the link connected to the pin
     */
    void deleteLink() override
    {
        if (auto l = m_link.lock()) {
            l->markForDeletion();
        }
        m_link.reset();
    }

    /**
     * @brief Specify if connections from an output on the same node are allowed
     * @param state New state of the flag
     */
    void allowSameNodeConnections(bool state)
    {
        m_allowSelfConnection = state;
    }

    /**
     * @brief <BR>Get connected status
     * @return [TRUE] is pin is connected to a link
     */
    bool isConnected() override
    {
        return !m_link.expired() && !m_link.lock()->isMarkedForDeletion();
    }

    /**
     * @brief <BR>Get pin's link
     * @return Weak_ptr reference to the link connected to the pin
     */
    std::weak_ptr<Link> getLink() override { return m_link; }

    /**
     * @brief <BR>Get InPin's connection filter
     * @return InPin's connection filter configuration
     */
    [[nodiscard]] const IFilter& getFilter() const { return *m_filter; }

    /**
     * @brief <BR>Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] const uint64_t getTypeID() const override
    {
        return detail::get_type_id<T>();
    };

    /**
     * @brief <BR>Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    ImVec2 pinPoint() override
    {
        return m_pos + ImVec2(-m_style->extra.socket_padding, m_size.y / 2);
    }

    void calculate() override;

    void prime() override;

    /**
     * @brief <BR>Get value carried by the connected link
     * @return Reference to the value of the connected OutPin. Or the default value if not connected
     */
    const T& val();

private:
    std::weak_ptr<Link> m_link;
    T m_emptyVal;
    T m_val;
    T m_valNext;
    std::shared_ptr<IFilter> m_filter;
    bool m_allowSelfConnection = false;
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
     * @brief <BR>Output pin constructor
     * @param name Name of the pin
     * @param filter Connection filter
     * @param parent Pointer to the Node containing the pin
     * @param inf Pointer to the Grid Handler the pin is in (same as parent)
     * @param style Style of the pin
     */
    explicit OutPin(
        PinUID uid,
        const std::string& name,
        std::shared_ptr<PinStyle> style,
        BaseNode* parent)
        : Pin(uid, name, style, PinType_Output, parent)
    {
    }

    /**
     * @brief <BR>When parent gets deleted, remove the links
     */
    ~OutPin() override;

    /**
     * @brief <BR>Create link between pins
     * @param other Pointer to the other pin
     */
    bool canCreateLink(Pin& other) override;

    /**
     * @brief <BR>Add a connected link to the internal list
     * @param link Pointer to the link
     */
    void setLink(std::weak_ptr<Link> link) override;

    /**
     * @brief <BR>Delete any expired weak pointers to a (now deleted) link
     */
    void deleteLink() override;

    /**
     * @brief <BR>Get connected status
     * @return [TRUE] is pin is connected to one or more links
     */
    bool isConnected() override { return !m_links.empty(); }

    /**
     * @brief <BR>Get pin's link attachment point (socket)
     * @return Grid coordinates to the attachment point between the link and the pin's socket
     */
    ImVec2 pinPoint() override
    {
        return m_pos + ImVec2(m_size.x + m_style->extra.socket_padding, m_size.y / 2);
    }

    void calculate() override;

    void prime() override;

    /**
     * @brief <BR>Get output value
     * @return Const reference to the internal value of the pin
     */
    const T& val();

    /**
     * @brief <BR>Set logic to calculate output value
     * @details Used to define the pin behaviour. This is what gets the data from the parent's inputs, and applies the needed logic.
     * @param func Function or lambda expression used to calculate output value
     */
    OutPin<T>* behaviour(std::function<T()> func)
    {
        m_behaviour = std::move(func);
        return this;
    }

    /**
     * @brief <BR>Get pin's data type (aka: \<T>)
     * @return String containing unique information identifying the data type
     */
    [[nodiscard]] const uint64_t getTypeID() const override
    {
        return detail::get_type_id<T>();
    };

private:
    std::vector<std::weak_ptr<Link>> m_links;
    std::function<T()> m_behaviour;
    T m_val;
    T m_valNext;
};

} // namespace ImFlow

#include "../src/ImNodeFlow.inl"

#endif
