#pragma once

#include "ImNodeFlow.h"

#include "imgui.h"

class NodeEditor {
public:
    NodeEditor(float width, float height);

    void set_size(ImVec2 d);

    void draw();

private:
    ImFlow::ImNodeFlow mINF;
};
