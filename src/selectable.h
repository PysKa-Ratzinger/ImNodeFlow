#pragma once

#include "./DoubleBuffer.h"

#include <cstdint>

namespace ImFlow::detail {

class Selectable;

class SelectableContext {
public:
    void prepareNextFrame();
    void count(Selectable const& selectable);

    uint64_t getTotalActivatedItems() const;
    uint64_t getTotalSelectedItems() const;

private:
    uint64_t _totalSelectedItems = 0;
    uint64_t _totalActivatedItems = 0;
};

class Selectable {
public:
    bool isActivated() const;
    bool isSelected() const;

    void setSelected(bool state);
    void setProvisionallySelected(bool state);

    void prepareNextFrame(SelectableContext& ctx);

    void on_click();
    void on_click_outside(bool ctrl_key_held = false);
    void on_release();
    void on_drag();

private:
    struct vars_t {
        bool selected;
        bool provisionally_selected;
    };

    bool _wants_deselect = false;
    bool _is_activated = false;
    DoubleBuffer<vars_t> _vars;
};

} // namespace ImFlow::detail
