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

class NotGate : public ImFlow::INodeBehaviour {
public:
    void setup(ImFlow::Node& node) override
    {
        node.setTitle("Not");
        node.setStyle(ImFlow::NodeStyles::red);
        node.addIN<bool>({
            .name = "IN",
            .default_value = false,
            .strat = ImFlow::PinConnStrat_AllowMultiplePreferOldest,
            .accumulate_op =
                [](bool first, bool init, bool new_input) -> bool {
                return first ? new_input : init | new_input;
            },
        });
        node.addOUT<bool>({
            .name = "OUT",
            .behaviour = [this](ImFlow::Node& node) -> int {
                return !node.inPin<bool>("IN")->value();
            },
        });
    }

    void draw(ImFlow::Node& node) override
    {
        auto& inPin = *node.inPin<bool>("IN");
        auto& outPin = *node.outPin<bool>("OUT");
        if (inPin.value()) {
            inPin.getStyle() = ImFlow::PinStyles::green;
            outPin.getStyle() = ImFlow::PinStyles::red;
        } else {
            inPin.getStyle() = ImFlow::PinStyles::red;
            outPin.getStyle() = ImFlow::PinStyles::green;
        }
    }

    std::unique_ptr<ImFlow::INodeBehaviour> clone() override
    {
        return std::make_unique<NotGate>();
    }
};

/* Node editor that sets up the grid to place nodes */
struct NodeEditor {
    ImFlow::ImNodeFlow mINF;

    NodeEditor(float width, float height)
    {
        mINF.setSize({ width, height });
        mINF.setSize({ width, height });
        mINF.rightClickPopUpContent([this](ImFlow::Node* node) -> void {
            if (node != nullptr) {
                ImGui::Text(
                    "you have right-clicked on [%s]",
                    node->getName().c_str());
            } else {
                bool should_close = false;
                ImVec2 mouse_screen_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
                if (ImGui::Button("SimpleSum")) {
                    mINF.emplaceNodeAt<SimpleSum>(mouse_screen_pos);
                    should_close = true;
                }
                if (ImGui::Button("ResultNode")) {
                    mINF.emplaceNodeAt<ResultNode>(mouse_screen_pos);
                    should_close = true;
                }
                if (ImGui::Button("MultipleInputAdder")) {
                    mINF.emplaceNodeAt<MultipleInputAdder>(mouse_screen_pos);
                    should_close = true;
                }
                if (ImGui::Button("CollapsingNode")) {
                    mINF.emplaceNodeAt<CollapsingNode>(mouse_screen_pos);
                    should_close = true;
                }
                if (ImGui::Button("Not Gate")) {
                    mINF.emplaceNodeAt<NotGate>(mouse_screen_pos);
                    should_close = true;
                }

                if (should_close) {
                    ImGui::CloseCurrentPopup();
                }
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

        // An example of how to make a digital circuit for a clock using only
        // not gates.
        // If this flashes too fast, the frame rate may be too high. Check you
        // have VSync enabled, or as a last resource, add more gates to slow
        // down the clock.
        std::vector<ImFlow::Node*> notGateChain;

        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                notGateChain.emplace_back(&mINF.emplaceNode<NotGate>({
                    float(50 + x * 90),
                    float(350 + y * 50),
                }));
            }
        }

        for (int i = 0; i < notGateChain.size(); i++) {
            notGateChain.at(i)->outPin("OUT")->linkTo(
                *notGateChain.at((i + 1) % notGateChain.size())->inPin("IN"));
        }
    }

    void set_size(ImVec2 d) { mINF.setSize(d); }

    void draw() { mINF.update(); }
};
