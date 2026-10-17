#pragma once

#include "ImNodeFlow.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

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
                return node.getInVal<int>("In") + _valB;
            },
        });
    }

    void draw(ImFlow::Node& node) override
    {
        ImGui::SetNextItemWidth(100.F);
        ImGui::InputInt("##ValB", &_valB);
    }

    std::unique_ptr<ImFlow::INodeBehaviour> clone() override
    {
        return std::make_unique<SimpleSum>();
    }

private:
    int _valB = 0;
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
                return node.getInVal<int>("In");
            },
        });
    };

    void draw(ImFlow::Node& node) override
    {
        ImGui::SetNextItemWidth(100.F);
        ImGui::Text("Result: %d", node.getInVal<int>("In"));
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

        // Addind inputs without a designated initializer list also
        // works, but linters will scream at you.
        //
        // NOLINTBEGIN (modernize-use-designated-initializers)
        node.addIN<int>({ "A", 0 });
        node.addIN<int>({ "B", 0 });
        // NOLINTEND

        node.addIN<int>({
            .name = "Carry",
            .default_value = 0,
            .renderer = [](auto* pin) -> void {
                ImGui::TextUnformatted("<CARRY>");
            },
        });

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
class NodeEditor {
public:
    NodeEditor(float width, float height)
    {
        m_flow.setSize({ width, height });
        m_flow.rightClickPopUpContent([](auto* node) {
            if (node != nullptr) {
                ImGui::Text(
                    "you have right-clicked on [%s]",
                    node->getName().c_str());
            } else {
                ImGui::TextUnformatted(
                    "you have right-clicked :)");
            }
        });

        auto& n1 = m_flow.emplaceNode<SimpleSum>({ 40, 40 });
        auto& n2 = m_flow.emplaceNode<SimpleSum>({ 40, 150 });
        auto& result = m_flow.emplaceNode<ResultNode>({ 270, 50 });
        auto& multi_adder = m_flow.emplaceNode<MultipleInputAdder>({ 320, 140 });

        result.setIndestructable(true);

        n1.outPin("Out").linkTo(result.inPin("A"));
        n2.outPin("Out").linkTo(result.inPin("B"));

        n1.outPin("Out").linkTo(multi_adder.inPin("In"));
        n2.outPin("Out").linkTo(multi_adder.inPin("In"));

        auto& collapsingNode = m_flow.emplaceNode<CollapsingNode>({ 350, 200 });
    }

    void set_size(ImVec2 d) { m_flow.setSize(d); }

    void draw() { m_flow.update(); }

    ImFlow::ImNodeFlow m_flow;
};
