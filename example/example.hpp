#pragma once

#include "ImNodeFlow.h"

class SimpleSum : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Simple sum");
        node.setStyle(ImFlow::NodeStyle::green());
        node.addIN<int>({ .name = "In", .default_value = 0 });
        node.addOUT<int>({
            .name = "Out",
            .behaviour = [this](ImFlow::Node& node) -> int {
                return node.getInVal<int>("In") + m_valB;
            },
        });
    }

    void draw(ImFlow::Node& node) override
    {
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputInt("##ValB", &m_valB);
    }

private:
    int m_valB = 0;
};

class CollapsingNode : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Collapsing node");
        node.setStyle(ImFlow::NodeStyle::red());
        node.addIN<int>({ .name = "A", .default_value = 0 });
        node.addIN<int>({ .name = "B", .default_value = 0 });
        node.addOUT<int>({
            .name = "Out",
            .behaviour = [this](ImFlow::Node& node) -> int {
                return node.getInVal<int>("A") + node.getInVal<int>("B");
            },
        });
    }

    void draw(ImFlow::Node& node) override
    {
        if (node.isSelected()) {
            ImGui::SetNextItemWidth(100.f);
            ImGui::Text(
                "You can only see me when the node is selected!");
        }
    }
};

class ResultNode : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Result node");
        node.setStyle(ImFlow::NodeStyle::brown());
        node.addIN<int>({ .name = "A", .default_value = 0 });
        node.addIN<int>({ .name = "B", .default_value = 0 });
    }

    void draw(ImFlow::Node& node) override
    {
        ImGui::Text(
            "Result: %d",
            node.getInVal<int>("A") + node.getInVal<int>("B"));
    }
};

/* Node editor that sets up the grid to place nodes */
struct NodeEditor {
    ImFlow::ImNodeFlow mINF;

    NodeEditor(float width, float height)
    {
        mINF.setSize({ width, height });

        auto& n1 = mINF.emplaceNode<SimpleSum>({ 40, 40 });
        auto& n2 = mINF.emplaceNode<SimpleSum>({ 40, 150 });
        auto& result = mINF.emplaceNode<ResultNode>({ 250, 80 });

        // Add links between nodes
        n1.outPin("Out")->createLink(result.inPin("A"));
        n2.outPin("Out")->createLink(result.inPin("B"));

        // Add a collapsing node
        auto& collapsingNode = mINF.emplaceNode<CollapsingNode>({ 300, 300 });
    }

    void set_size(ImVec2 d) { mINF.setSize(d); }

    void draw() { mINF.update(); }
};
