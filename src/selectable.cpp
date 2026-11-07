#include "selectable.h"

namespace ImFlow::detail {

void SelectableContext::prepareNextFrame()
{
    _totalSelectedItems = 0;
    _totalActivatedItems = 0;
}

void SelectableContext::count(Selectable const& selectable)
{
    if (selectable.isSelected()) {
        ++_totalSelectedItems;
    }
    if (selectable.isActivated()) {
        ++_totalActivatedItems;
    }
}

uint64_t SelectableContext::getTotalSelectedItems() const
{
    return _totalSelectedItems;
}

bool Selectable::isActivated() const
{
    return _is_activated;
}

bool Selectable::isSelected() const
{
    return _vars.value().selected || _vars.value().provisionally_selected;
}

void Selectable::setSelected(bool state)
{
    _vars.next().selected = state;
}

void Selectable::setProvisionallySelected(bool state)
{
    _vars.next().provisionally_selected = state;
}

void Selectable::prepareNextFrame(SelectableContext& ctx)
{
    _is_activated = !_vars.value().selected && _vars.next().selected;
    _vars.swap();
    _vars.next().provisionally_selected = false;

    ctx.count(*this);
}

void Selectable::on_click()
{
    _wants_deselect = _vars.value().selected;
    setSelected(true);
}

void Selectable::on_click_outside(bool ctrl_key_held)
{
    if (!ctrl_key_held) {
        _wants_deselect = false;
        setSelected(false);
    }
}

void Selectable::on_release()
{
    if (_wants_deselect) {
        setSelected(false);
    }
}

void Selectable::on_drag()
{
    _wants_deselect = false;
}

} // namespace ImFlow::detail
