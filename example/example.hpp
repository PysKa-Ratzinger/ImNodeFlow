#pragma once

#include "ImNodeFlow.h"

class SimpleSum : public ImFlow::INode {
public:
    explicit SimpleSum(ImFlow::BaseNode& node)
    {
        node.setTitle("Simple sum");
        node.setStyle(ImFlow::NodeStyle::green());
        node.addIN<int>("In", 0, ImFlow::ConnectionFilter::SameType());
        node.addOUT<int>("Out", nullptr).behaviour([&]() -> int {
            return node.getInVal<int>("In") + m_valB;
        });
    }

    void draw(ImFlow::BaseNode& node) override
    {
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputInt("##ValB", &m_valB);
    }

private:
    int m_valB = 0;
};

class CollapsingNode : public ImFlow::INode {
public:
    explicit CollapsingNode(ImFlow::BaseNode& node)
    {
        node.setTitle("Collapsing node");
        node.setStyle(ImFlow::NodeStyle::red());
        node.addIN<int>("A", 0, ImFlow::ConnectionFilter::SameType());
        node.addIN<int>("B", 0, ImFlow::ConnectionFilter::SameType());
        node.addOUT<int>("Out", nullptr).behaviour([&]() -> int {
            return node.getInVal<int>("A") + node.getInVal<int>("B");
        });
    }

    void draw(ImFlow::BaseNode& node) override
    {
        if (node.isSelected()) {
            ImGui::SetNextItemWidth(100.f);
            ImGui::Text(
                "You can only see me when the node is selected!");
        }
    }
};

class ResultNode : public ImFlow::INode {
public:
    explicit ResultNode(ImFlow::BaseNode& node)
    {
        node.setTitle("Result node");
        node.setStyle(ImFlow::NodeStyle::brown());
        node.addIN<int>("A", 0, ImFlow::ConnectionFilter::SameType());
        node.addIN<int>("B", 0, ImFlow::ConnectionFilter::SameType());
    }

    void draw(ImFlow::BaseNode& node) override
    {
        ImGui::Text(
            "Result: %d",
            node.getInVal<int>("A") + node.getInVal<int>("B"));
    }
};

/* Node editor that sets up the grid to place nodes */
struct NodeEditor {
    ImFlow::ImNodeFlow mINF;

    NodeEditor(float d, std::size_t r)
    {
        mINF.setSize({ d, d });
        if (r > 0) {
            auto n1 = mINF.addNode<SimpleSum>({ 40, 40 });
            auto n2 = mINF.addNode<SimpleSum>({ 40, 150 });
            auto result = mINF.addNode<ResultNode>({ 250, 80 });

            // Add links between nodes
            n1->outPin("Out").linkTo(result->inPin("A"));
            n2->outPin("Out").linkTo(result->inPin("B"));

            // Add a collapsing node
            auto collapsingNode = mINF.addNode<CollapsingNode>({ 300, 300 });
        }
    }

    void set_size(ImVec2 d) { mINF.setSize(d); }

    void draw() { mINF.update(); }
};
