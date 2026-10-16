#pragma once

#include "ImNodeFlow.h"

class SimpleSum : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Simple sum");
        node.setStyle(ImFlow::NodeStyles::green);
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
        node.setStyle(ImFlow::NodeStyles::red);

        // Addind inputs without a designated initializer list also
        // works, but linters will scream at you.
        //
        // NOLINTBEGIN (modernize-use-designated-initializers)
        node.addIN<int>({ "A", 0 });
        node.addIN<int>({ "B", 0 });
        // NOLINTEND

        node.addOUT<int>({
            .name = "Out",
            .behaviour = [this](ImFlow::Node& node) -> int {
                return node.getInVal<int>("A") + node.getInVal<int>("B");
            },
            .renderer = [](auto* pin) -> void {
                ImGui::TextUnformatted("<OUTPIN>");
            },
        });
    }

    void draw(ImFlow::Node& node) override
    {
        ImGui::TextUnformatted("Select me to see more");

        if (node.isSelected()) {
            ImGui::SetNextItemWidth(100.F);
            ImGui::Text(
                "You can only see me when the node is selected!");

            const char* multiplier_label = m_multiplier ? "On" : "Off";
            ImGui::SetNextItemWidth(100.F);
            if (ImGui::BeginCombo("Multiplier", multiplier_label)) {
                if (ImGui::Selectable("Off", !m_multiplier)) {
                    m_multiplier = false;
                }
                if (ImGui::Selectable("On", m_multiplier)) {
                    m_multiplier = true;
                }
                ImGui::EndCombo();
            }
        }

        if (m_multiplier) {
            node.showOUT<int>({
                .name = "Multiplier",
                .behaviour = [](ImFlow::Node& node) -> int {
                    return node.getInVal<int>("A") * node.getInVal<int>("B");
                },
            });
        }
    }

private:
    bool m_multiplier = false;
};

class ResultNode : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Result node");
        node.setStyle(ImFlow::NodeStyles::brown);
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
        n1.outPin("Out")->linkTo(result.inPin("A"));
        n2.outPin("Out")->linkTo(result.inPin("B"));

        // Add a collapsing node
        auto& collapsingNode = mINF.emplaceNode<CollapsingNode>({ 300, 300 });
    }

    void set_size(ImVec2 d) { mINF.setSize(d); }

    void draw() { mINF.update(); }
};
