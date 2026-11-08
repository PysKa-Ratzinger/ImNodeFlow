#pragma once

#include "./DoubleBuffer.h"

#include <cstdint>

namespace ImFlow::detail {

class Selectable;

class SelectableContext {
public:
    void deselectAll();
    uint64_t serial() const;

private:
    uint64_t _serial = 0;
};

class Selectable {
public:
    explicit Selectable(SelectableContext& ctx);

    bool isActivated(SelectableContext const& ctx) const;
    bool isSelected(SelectableContext const& ctx) const;

    void setSelected(SelectableContext& ctx, bool state);
    void setProvisionallySelected(SelectableContext& ctx, bool state);

    void prepareNextFrame(SelectableContext& ctx);

    void update(
        SelectableContext& ctx,
        bool is_ctrl_held_down,
        bool is_clicked,
        bool is_released,
        bool is_dragged);

    void on_click(SelectableContext& ctx, bool is_ctrl_held_down);
    void on_release(SelectableContext& ctx, bool is_ctrl_held_down);
    void on_drag(SelectableContext& ctx);

private:
    void updateSerials(SelectableContext& ctx);

    struct vars_t {
        uint64_t selected;
        uint64_t provisionally_selected;
    };

    bool _wants_deselect = false;
    bool _is_activated = false;
    DoubleBuffer<vars_t> _vars;
};

} // namespace ImFlow::detail
