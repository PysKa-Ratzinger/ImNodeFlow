#ifndef IMNODEFLOW_ICONTAINEDCONTEXT_H
#define IMNODEFLOW_ICONTAINEDCONTEXT_H
#pragma once

#include "imgui.h"

namespace ImFlow {

struct ContainedContextConfig {
    bool extra_window_wrapper = false;
    ImVec2 size = { 0.f, 0.f };
    ImU32 color = IM_COL32_WHITE;
    bool zoom_enabled = true;
    float zoom_min = 0.3f;
    float zoom_max = 2.f;
    float zoom_divisions = 10.f;
    float zoom_smoothness = 5.f;
    float default_zoom = 1.f;
    ImGuiKey reset_zoom_key = ImGuiKey_R;
    ImGuiMouseButton selection_box_button = ImGuiMouseButton_Left;
    ImGuiMouseButton selection_box_cancel_button = ImGuiMouseButton_Right;
    ImGuiMouseButton scroll_button = ImGuiMouseButton_Middle;

    ImGuiWindowFlags extra_viewport_window_flags = 0;
};

class IContainedContext {
public:
    virtual ~IContainedContext();
    virtual ContainedContextConfig& config() = 0;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void updateScroll() = 0;
    [[nodiscard]] virtual ImVec2 size() const = 0;
    [[nodiscard]] virtual float scale() const = 0;
    [[nodiscard]] virtual const ImVec2& origin() const = 0;
    [[nodiscard]] virtual bool hovered() const = 0;
    [[nodiscard]] virtual const ImVec2& scroll() const = 0;
    virtual void setScroll(ImVec2 scroll) = 0;
    virtual void setScale(float scale) = 0;
    [[nodiscard]] virtual ImVec2 getScreenDelta() = 0;
    virtual ImGuiContext* getRawContext() = 0;
    virtual void setFontDensity() = 0;
    virtual void sampleHostFontDensity() = 0;
};

} // namespace ImFlow

#endif // IMNODEFLOW_ICONTAINEDCONTEXT_H
