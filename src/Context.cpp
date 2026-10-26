#include "ImNodeFlow/Context.h"

#include "ImNodeFlow/NodeRef.h"
#include "ImNodeFlow/PinRef.h"
#include "ImNodeFlow/PinType.h"

#include "./Link.h"
#include "./editor_context.h"
#include "./imgui_bezier_items.h"
#include "./pin_base.h"
#include "./uid_helpers.h"

#include <memory>

namespace ImFlow {

namespace {

    /**
     * @brief Helper struct for creating a node from a lambda
     */
    struct NodeWrapper : public INodeBehaviour {
        lambda_draw_func_t m_draw_func;

        NodeWrapper(lambda_draw_func_t draw_func)
            : m_draw_func(std::move(draw_func))
        {
        }

        void draw(NodeRef node) override { m_draw_func(node); }
    };

} // anonymous namespace

int ImNodeFlow::m_instances = 0;

NodeRef ImNodeFlow::addNode(const ImVec2& grid_coords, NodeInfo node_info)
{
    auto uid = detail::generate_node_uid();
    auto res = m_impl->nodes.emplace(
        uid,
        detail::node_t {
            .ctx = *this,
            .uid = uid,
            .pos = grid_coords,
            .posTarget = grid_coords,
            .style = std::move(node_info.style),
            .behaviour = std::move(node_info.behaviour),
        });

    assert(res.second && "Failed to create new Node");
    detail::node_t& node = res.first->second;

    if (node.behaviour) {
        node.behaviour->setup(NodeRef { node });
    }

    return NodeRef { node };
}

NodeRef ImNodeFlow::placeNodeAt(const ImVec2& screen_coords, NodeInfo node_info)
{
    return addNode(screen2grid(screen_coords), std::move(node_info));
}

NodeRef ImNodeFlow::placeNode(NodeInfo node_info)
{
    return placeNodeAt(ImGui::GetMousePos(), std::move(node_info));
}

NodeRef ImNodeFlow::addNodeLambda(const ImVec2& grid_coords, lambda_draw_func_t draw_func)
{
    return addNode(
        grid_coords,
        NodeInfo {
            .behaviour = std::make_unique<NodeWrapper>(std::move(draw_func)),
        });
}

ImNodeFlow::ImNodeFlow()
    : ImNodeFlow("FlowGrid" + std::to_string(m_instances))
{
}

ImNodeFlow::ImNodeFlow(std::string name)
    : m_impl(std::make_unique<detail::editor_context_t>(
          detail::editor_context_t {
              .name { std::move(name) },
          }))
{
    m_instances++;
    m_impl->context.config().extra_window_wrapper = true;
    m_impl->context.config().color = m_impl->style.colors.background;
}

ImNodeFlow::ImNodeFlow(ImNodeFlow&&) noexcept = default;
ImNodeFlow& ImNodeFlow::operator=(ImNodeFlow&&) = default;

ImNodeFlow::~ImNodeFlow() = default;

bool ImNodeFlow::mouse_on_selected_node()
{
    return std::any_of(m_impl->nodes.begin(), m_impl->nodes.end(), [](const auto& n) {
        ConstNodeRef node { n.second };
        return node.isSelected() && node.isHovered();
    });
}

bool ImNodeFlow::mouse_on_free_space()
{
    return std::all_of(
               m_impl->nodes.begin(),
               m_impl->nodes.end(),
               [](const auto& n) { return !ConstNodeRef { n.second }.isHovered(); })
        && std::all_of(m_impl->links.begin(), m_impl->links.end(), [](const auto& l) {
               return !l->isHovered();
           });
}

ImVec2
ImNodeFlow::screen2grid(const ImVec2& p)
{
    if (ImGui::GetCurrentContext() == m_impl->context.getRawContext())
        return p - m_impl->context.scroll();
    return (p - m_impl->context.origin()) / m_impl->context.scale() - m_impl->context.scroll();
}

ImVec2
ImNodeFlow::grid2screen(const ImVec2& p)
{
    if (ImGui::GetCurrentContext() == m_impl->context.getRawContext())
        return p + m_impl->context.scroll();
    return (p + m_impl->context.scroll()) * m_impl->context.scale() + m_impl->context.origin();
}

void ImNodeFlow::createLink(PinRef<> source_pin, PinRef<> target_pin)
{
    if (source_pin == target_pin) {
        return;
    }

    if (source_pin.getType() == target_pin.getType()) {
        // Cannot link inputs to inputs nor outputs to outputs
        return;
    }

    bool same_node_allowed = source_pin.allowsSameNodeConnections()
        && target_pin.allowsSameNodeConnections();
    if (source_pin.getParent() == target_pin.getParent() && !same_node_allowed) {
        return;
    }

    for (const auto& pin_ref : source_pin.getConnections()) {
        if (pin_ref == target_pin) {
            // Connection already exists
            return;
        }
    }

    // Only pins with the same type can be linked. Casts between
    // compatible types like (float and double), or (short, int, and long)
    // should be handled explicitly by user types.
    //
    // On a technical note, supporting these casts is not necessarily hard,
    // but introduces a layer of complexity which either requires RTTI (so
    // we know how to type-cast), or manual RTTI (using a compile-time
    // type_id<T>, which is possible, but we are now responsible for
    // maintaining a list of which type conversions should be possible and
    // which aren't), or it might introduce subtle programming errors
    // given the nature that different types might naturally have different
    // sizes in memory, and getting a cast wrong may result in writing
    // into out-of-bounds memory.
    //
    // Supporting connections only between same types avoids all these
    // issues, and it's not something that's prohibitibly hard to work
    // around on the client side.
    if (source_pin.getTypeID() != target_pin.getTypeID()) {
        return;
    }

    if (source_pin.getType() == PinType::Output
        && target_pin.getType() == PinType::Input) {

        m_impl->links.emplace_back(
            std::make_unique<detail::Link>(
                source_pin,
                target_pin));

    } else if (source_pin.getType() == PinType::Input
        && target_pin.getType() == PinType::Output) {

        m_impl->links.emplace_back(
            std::make_unique<detail::Link>(
                target_pin,
                source_pin));
    }
}

void ImNodeFlow::droppedLinkPopUpContent(
    dropped_link_popup_handler_t handler,
    ImGuiKey key)
{
    m_impl->droppedLinkPopUp = std::move(handler);
    m_impl->droppedLinkPupUpComboKey = key;
}

void ImNodeFlow::rightClickPopUpContent(right_click_popup_handler_t content)
{
    m_impl->rightClickPopUp = std::move(content);
}

void ImNodeFlow::dragDropTarget(drag_drop_target_handler_t content)
{
    m_impl->dragdropTarget = std::move(content);
}

bool ImNodeFlow::getSingleUseClick(ImGuiMouseButton button) const
{
    switch (button) {
    case ImGuiMouseButton_Left:
        return m_impl->singleUseClicks.left;
    case ImGuiMouseButton_Right:
        return m_impl->singleUseClicks.right;
    case ImGuiMouseButton_Middle:
        return m_impl->singleUseClicks.middle;
    default:
        return false;
    }
}

void ImNodeFlow::consumeSingleUseClick(ImGuiMouseButton button)
{
    switch (button) {
    case ImGuiMouseButton_Left:
        m_impl->singleUseClicks.left = false;
        return;

    case ImGuiMouseButton_Right:
        m_impl->singleUseClicks.right = false;
        return;

    case ImGuiMouseButton_Middle:
        m_impl->singleUseClicks.middle = false;
        return;

    default:
        return;
    }
}

const std::string& ImNodeFlow::getName() const
{
    return m_impl->name;
}

const ImVec2& ImNodeFlow::getPos() const
{
    return m_impl->context.origin();
}

const ImVec2& ImNodeFlow::getScroll() const
{
    return m_impl->context.scroll();
}

void ImNodeFlow::setScroll(ImVec2 scroll)
{
    m_impl->context.setScroll(scroll);
}

void ImNodeFlow::setScale(float scale)
{
    m_impl->context.setScale(scale);
}

ImVec2 ImNodeFlow::getScreenSpaceDelta() const
{
    return m_impl->context.getScreenDelta();
}

uint64_t ImNodeFlow::getNodesCount() const
{
    return m_impl->nodes.size();
}

IContainedContext& ImNodeFlow::getGrid()
{
    return m_impl->context;
}

[[nodiscard]] bool ImNodeFlow::isNodeDragged() const
{
    return m_impl->draggingNode;
}

const InfStyler& ImNodeFlow::getStyle() const
{
    return m_impl->style;
}

void ImNodeFlow::setStyle(InfStyler style)
{
    m_impl->style = std::move(style);
}

void ImNodeFlow::setSize(const ImVec2& size)
{
    m_impl->context.config().size = size;
}

void ImNodeFlow::draggingNode(bool state)
{
    m_impl->draggingNodeNext = state;
}

void ImNodeFlow::hovering(detail::pin_base_t* hovering)
{
    m_impl->hovering = hovering;
}

void ImNodeFlow::dragging(detail::pin_base_t* dragged)
{
    m_impl->dragOut = dragged;
}

void ImNodeFlow::hoveredNode(detail::node_t* hovering)
{
    m_impl->hoveredNode = hovering;
}

void ImNodeFlow::update()
{
    // Updating looping stuff
    m_impl->hovering = nullptr;
    m_impl->hoveredNode = nullptr;
    m_impl->draggingNode = m_impl->draggingNodeNext;

    m_impl->singleUseClicks.left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    m_impl->singleUseClicks.right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    m_impl->singleUseClicks.middle = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);

    // Create child canvas
    m_impl->context.begin();
    ImGui::GetIO().IniFilename = nullptr;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Display grid
    ImVec2 gridSize = ImGui::GetWindowSize();
    float subGridStep = m_impl->style.grid_size / m_impl->style.grid_subdivisions;
    for (float x = fmodf(m_impl->context.scroll().x, m_impl->style.grid_size);
        x < gridSize.x;
        x += m_impl->style.grid_size)
        draw_list->AddLine(
            ImVec2(x, 0.0f),
            ImVec2(x, gridSize.y),
            m_impl->style.colors.grid);
    for (float y = fmodf(m_impl->context.scroll().y, m_impl->style.grid_size);
        y < gridSize.y;
        y += m_impl->style.grid_size)
        draw_list->AddLine(
            ImVec2(0.0f, y),
            ImVec2(gridSize.x, y),
            m_impl->style.colors.grid);
    if (m_impl->context.scale() > 0.7f) {
        for (float x = fmodf(m_impl->context.scroll().x, subGridStep);
            x < gridSize.x;
            x += subGridStep)
            draw_list->AddLine(
                ImVec2(x, 0.0f),
                ImVec2(x, gridSize.y),
                m_impl->style.colors.subGrid);
        for (float y = fmodf(m_impl->context.scroll().y, subGridStep);
            y < gridSize.y;
            y += subGridStep)
            draw_list->AddLine(
                ImVec2(0.0f, y),
                ImVec2(gridSize.x, y),
                m_impl->style.colors.subGrid);
    }

    // Propagate pin values
    for (auto& node : m_impl->nodes) {
        NodeRef { node.second }.calculate();
    }
    for (auto& node : m_impl->nodes) {
        NodeRef { node.second }.prime();
    }

    // Update and draw nodes
    for (auto& node : m_impl->nodes) {
        NodeRef { node.second }.update();
    }

    // Remove "toDelete" nodes
    for (auto iter = m_impl->nodes.begin(); iter != m_impl->nodes.end();) {
        if (NodeRef { iter->second }.toDestroy())
            iter = m_impl->nodes.erase(iter);
        else
            ++iter;
    }
    for (auto& node : m_impl->nodes) {
        NodeRef { node.second }.updatePublicStatus();
    }

    // Update and draw links
    for (auto iter = m_impl->links.begin(); iter != m_impl->links.end();) {
        // Clean up deleted links along the way
        if ((*iter)->isMarkedForDeletion()) {
            iter = m_impl->links.erase(iter);
            continue;
        }

        (*iter)->update(*this);
        ++iter;
    }

    if (m_impl->dragOut != nullptr) {
        PinRef<> dragOut { *m_impl->dragOut };

        // Links drop-off
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (m_impl->hovering != nullptr) {
                dragOut.linkTo(PinRef<> { *m_impl->hovering });
            } else if (
                mouse_on_free_space() && m_impl->droppedLinkPopUp && (m_impl->droppedLinkPupUpComboKey == ImGuiKey_None || ImGui::IsKeyDown(m_impl->droppedLinkPupUpComboKey))) {
                m_impl->droppedLinkLeft = m_impl->dragOut;
                ImGui::OpenPopup("DroppedLinkPopUp");
            }
        }

        // Links drag-out
        if (dragOut.getType() == PinType::Output)
            detail::smart_bezier(
                dragOut.pinPoint(),
                ImGui::GetMousePos(),
                dragOut.getStyle().color,
                dragOut.getStyle().extra.link_dragged_thickness);
        else
            detail::smart_bezier(
                ImGui::GetMousePos(),
                dragOut.pinPoint(),
                dragOut.getStyle().color,
                dragOut.getStyle().extra.link_dragged_thickness);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            m_impl->dragOut = nullptr;
    }

    if (this->mouse_on_free_space() && !m_impl->box.active && !m_impl->draggingNode && !m_impl->dragOut && getSingleUseClick(getGrid().config().selection_box_button) && ImGui::IsWindowHovered()) {
        consumeSingleUseClick(getGrid().config().selection_box_button);
        m_impl->box.active = true;
        m_impl->box.selectionStart = ImGui::GetMousePos();

        // Deselect all nodes if not holding down CTRL
        if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
            for (auto& node : m_impl->nodes) {
                NodeRef { node.second }.setSelected(false);
            }
        }
    }

    if (m_impl->box.active) {
        ImRect boxClip { m_impl->box.selectionStart, m_impl->box.selectionStart };
        boxClip.Add(ImGui::GetMousePos());

        draw_list->AddRectFilled(
            boxClip.GetBL(), boxClip.GetTR(), m_impl->box.selectionColor);
        draw_list->AddRect(
            boxClip.GetBL(),
            boxClip.GetTR(),
            m_impl->box.borderColor,
            0.0F,
            0,
            1.5F);

        bool shouldMarkSelected = false;

        if (ImGui::IsMouseReleased(
                getGrid().config().selection_box_button)) {
            m_impl->box.active = false;
            shouldMarkSelected = true;
        } else if (
            getGrid().config().selection_box_cancel_button != getGrid().config().selection_box_button && ImGui::IsMouseClicked(getGrid().config().selection_box_cancel_button)) {
            // cancel selection
            m_impl->box.active = false;
        }

        for (auto& elem : m_impl->nodes) {
            NodeRef node { elem.second };

            ImVec2 nodePos = grid2screen(node.getTLPos());
            ImVec2 nodeSize = node.getFullSize();

            ImRect nodeClip { nodePos, nodePos + nodeSize };
            if (boxClip.Overlaps(nodeClip)) {
                if (shouldMarkSelected) {
                    node.setSelected(true);
                } else {
                    node.setProvisionallySelected(true);
                }
            }
        }
    }

    // Right-click PopUp
    if (m_impl->rightClickPopUp && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
        m_impl->hoveredNodeAux = m_impl->hoveredNode;
        ImGui::OpenPopup("RightClickPopUp");
    }
    if (ImGui::BeginPopup("RightClickPopUp")) {
        std::optional<NodeRef> arg;
        if (m_impl->hoveredNodeAux != nullptr) {
            arg.emplace(*m_impl->hoveredNodeAux);
        }
        m_impl->rightClickPopUp(std::move(arg));
        ImGui::EndPopup();
    }

    // Dropped Link PopUp
    if (ImGui::BeginPopup("DroppedLinkPopUp")) {
        m_impl->droppedLinkPopUp(PinRef<> { *m_impl->droppedLinkLeft });
        ImGui::EndPopup();
    }

    if (m_impl->context.hovered() && this->getSingleUseClick(m_impl->context.config().scroll_button)) {
        this->consumeSingleUseClick(m_impl->context.config().scroll_button);
        m_impl->draggingCanvas = true;
    }

    if (m_impl->draggingCanvas) {
        if (!ImGui::IsMouseDown(m_impl->context.config().scroll_button)) {
            m_impl->draggingCanvas = false;
        }
    }

    if (m_impl->draggingCanvas && ImGui::IsMouseDragging(m_impl->context.config().scroll_button)) {
        m_impl->context.updateScroll();
    }

    m_impl->context.end();

    if (ImGui::BeginDragDropTarget()) {
        if (m_impl->dragdropTarget) {
            m_impl->dragdropTarget(screen2grid(ImGui::GetMousePos()));
        }

        ImGui::EndDragDropTarget();
    }
}

} // namespace ImFlow
