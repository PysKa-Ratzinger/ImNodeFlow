#pragma once

#include "ImNodeFlow.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

// Wrapper for common operations among the example's nodes
class SimpleNodeWrapper {
public:
    // Delete all these operations to avoid accidentally copying or moving
    // this class
    SimpleNodeWrapper(const SimpleNodeWrapper&) = delete;
    SimpleNodeWrapper(SimpleNodeWrapper&&) = delete;
    SimpleNodeWrapper& operator=(const SimpleNodeWrapper&) = delete;
    SimpleNodeWrapper& operator=(SimpleNodeWrapper&&) = delete;

    SimpleNodeWrapper(ImFlow::ImNodeFlow& flow, ImVec2 pos)
        : _node(flow.addNode(pos))
    {
        // Register ourselves as the deferred draw operation
        node().on_draw([this]() -> void { this->draw(); });

        // Listen to cleanup events to prevent accessing the node
        // after it's destruction which is controlled by the library
        node().on_cleanup([this]() -> void { _node.reset(); });
    }

    virtual ~SimpleNodeWrapper()
    {
        if (_node.has_value()) {
            // Clear all callbacks since they are no longer valid
            node().on_draw();
            node().on_cleanup();

            // This notifies ImNodeFlow that the node should be
            // destroyed
            node().destroy();
        }
    }

    /// Shoft-hand accessor
    [[nodiscard]] auto node() const -> ImFlow::BaseNode&
    {
        return _node.value().get();
    }

    /// To be implemented by the specific nodes in this example
    virtual void draw() = 0;

    [[nodiscard]] bool valid() const { return _node.has_value(); }

private:
    std::optional<std::reference_wrapper<ImFlow::BaseNode>> _node;
};

class SimpleSum : public SimpleNodeWrapper {
public:
    SimpleSum(ImFlow::ImNodeFlow& flow, ImVec2 pos)
        : SimpleNodeWrapper(flow, pos)
    {
        node().setTitle("Simple sum");
        node().setStyle(ImFlow::NodeStyle::green());
        node().addIN<int>("In", 0, ImFlow::ConnectionFilter::SameType());
        node().addOUT<int>("Out").behaviour([this]() -> int {
            return node().getInVal<int>("In") + _valB;
        });
    }

    void draw() final
    {
        ImGui::SetNextItemWidth(100.F);
        ImGui::InputInt("##ValB", &_valB);
    }

private:
    int _valB = 0;
};

class CollapsingNode : public SimpleNodeWrapper {
public:
    CollapsingNode(ImFlow::ImNodeFlow& flow, ImVec2 pos)
        : SimpleNodeWrapper(flow, pos)
    {
        node().setTitle("Collapsing node");
        node().setStyle(ImFlow::NodeStyle::red());
        node().addIN<int>("A", 0, ImFlow::ConnectionFilter::SameType());
        node().addIN<int>("B", 0, ImFlow::ConnectionFilter::SameType());
        node().addOUT<int>("Out").behaviour([this]() -> int {
            return node().getInVal<int>("A") + node().getInVal<int>("B");
        });
    }

    void draw() final
    {
        if (node().isSelected()) {
            ImGui::SetNextItemWidth(100.f);
            ImGui::Text(
                "You can only see me when the node is selected!");
        }
    }
};

class ResultNode : public SimpleNodeWrapper {
public:
    ResultNode(ImFlow::ImNodeFlow& flow, ImVec2 pos)
        : SimpleNodeWrapper(flow, pos)
    {
        node().setTitle("Result node");
        node().setStyle(ImFlow::NodeStyle::brown());
        node().addIN<int>("A", 0, ImFlow::ConnectionFilter::SameType());
        node().addIN<int>("B", 0, ImFlow::ConnectionFilter::SameType());
    }

    void draw() final
    {
        ImGui::Text(
            "Result: %d",
            node().getInVal<int>("A") + node().getInVal<int>("B"));
    }
};

/* Node editor that sets up the grid to place nodes */
class NodeEditor {
public:
    NodeEditor(float width, float height)
    {
        m_flow.setSize({ width, height });

        auto& n1 = add<SimpleSum>({ 40, 40 });
        auto& n2 = add<SimpleSum>({ 40, 150 });
        auto& result = add<ResultNode>({ 250, 80 });

        n1.node().outPin("Out").linkTo(result.node().inPin("A"));
        n2.node().outPin("Out").linkTo(result.node().inPin("B"));

        // Add a collapsing node
        auto& collapsingNode = add<CollapsingNode>({ 300, 300 });
    }

    ~NodeEditor() { m_my_nodes.clear(); }

    void set_size(ImVec2 d) { m_flow.setSize(d); }

    /// @brief Short-hand creator for the example nodes
    template <typename T>
    T& add(ImVec2 pos)
    {
        auto node = std::make_unique<T>(m_flow, pos);
        auto* raw_ptr = node.get();
        m_my_nodes.emplace_back(std::move(node));
        return *raw_ptr;
    }

    void draw()
    {
        m_flow.update();

        m_my_nodes.erase(
            std::remove_if(
                m_my_nodes.begin(),
                m_my_nodes.end(),
                [](const std::unique_ptr<SimpleNodeWrapper>&
                        holder) -> bool {
                    return !holder->valid();
                }),
            m_my_nodes.end());
    }

    ImFlow::ImNodeFlow m_flow;
    std::vector<std::unique_ptr<SimpleNodeWrapper>> m_my_nodes;
};
