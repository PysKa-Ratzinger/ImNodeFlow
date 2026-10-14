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
                return node.inPin<int>("In")->value() + m_valB;
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

class MultipleInputAdder : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Multiple input adder");
        node.setStyle(ImFlow::NodeStyles::cyan);
        node.addIN<int>({
            .name = "In",
            .default_value = 0,
            .strat = ImFlow::PinConnStrat_AllowMultiplePreferOldest,
            .accumulate_op =
                [](bool first, int init, int new_input) -> int {
                return first ? new_input : init + new_input;
            },
        });
        node.addOUT<int>({
            .name = "Out",
            .behaviour = [](ImFlow::Node& node) -> int {
                return node.inPin<int>("In")->value();
            },
        });
    };

    void draw(ImFlow::Node& node) override
    {
        ImGui::SetNextItemWidth(100.F);
        ImGui::Text("Result: %d", node.inPin<int>("In")->value());
    }

    std::unique_ptr<ImFlow::INodeBehaviour> clone() override
    {
        return std::make_unique<MultipleInputAdder>();
    }
};

class CollapsingNode : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Collapsing node");
        node.setStyle(ImFlow::NodeStyles::red);

        node.addIN<int>({
            .name = "A",
            .default_value = 0,
            .style = ImFlow::PinStyles::red,
        });
        node.addIN<int>({
            .name = "B",
            .default_value = 0,
            .style = ImFlow::PinStyles::red,
        });

        node.addOUT<int>({
            .name = "Out",
            .behaviour = [this](ImFlow::Node& node) -> int {
                return node.inPin<int>("A")->value() + node.inPin<int>("B")->value();
            },
            .renderer = [](auto* pin) -> void {
                ImGui::TextUnformatted("<OUTPIN>");
            },
            .style = ImFlow::PinStyles::red,
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
                    return node.inPin<int>("A")->value() * node.inPin<int>("B")->value();
                },
                .style = ImFlow::PinStyles::red,
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
            node.inPin<int>("A")->value() + node.inPin<int>("B")->value());
    }
};

/* Node editor that sets up the grid to place nodes */
struct NodeEditor {
    ImFlow::ImNodeFlow mINF;

    NodeEditor(float width, float height)
    {
        mINF.setSize({ width, height });
        mINF.setSize({ width, height });
        mINF.rightClickPopUpContent([](auto* node) -> void {
            if (node != nullptr) {
                ImGui::Text(
                    "you have right-clicked on [%s]",
                    node->getName().c_str());
            } else {
                ImGui::TextUnformatted(
                    "you have right-clicked :)");
            }
        });

        auto& n1 = mINF.emplaceNode<SimpleSum>({ 40, 40 });
        auto& n2 = mINF.emplaceNode<SimpleSum>({ 40, 150 });
        auto& result = mINF.emplaceNode<ResultNode>({ 340, 20 });
        auto& multi_adder = mINF.emplaceNode<MultipleInputAdder>({ 360, 140 });

        result.setIndestructable(true);

        // Add links between nodes
        n1.outPin("Out")->linkTo(*result.inPin("A"));
        n2.outPin("Out")->linkTo(*result.inPin("B"));

        n1.outPin("Out")->linkTo(*multi_adder.inPin("In"));
        n2.outPin("Out")->linkTo(*multi_adder.inPin("In"));

        // Add a collapsing node
        auto& collapsingNode = mINF.emplaceNode<CollapsingNode>({ 330, 230 });

        n1.outPin("Out")->linkTo(*collapsingNode.inPin("A"));
        n2.outPin("Out")->linkTo(*collapsingNode.inPin("B"));
    }

    void set_size(ImVec2 d) { mINF.setSize(d); }

    void draw() { mINF.update(); }
};
