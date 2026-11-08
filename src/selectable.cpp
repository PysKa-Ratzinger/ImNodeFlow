#include "selectable.h"

namespace ImFlow::detail {

void SelectableContext::deselectAll()
{
    ++_serial;
}

uint64_t SelectableContext::serial() const
{
    return _serial;
}

Selectable::Selectable(SelectableContext& ctx)
    : _vars(vars_t {
          .selected = ctx.serial() - 1,
          .provisionally_selected = ctx.serial() - 1,
      })
{
}

bool Selectable::isActivated(SelectableContext const& ctx) const
{
    return _is_activated;
}

bool Selectable::isSelected(SelectableContext const& ctx) const
{
    return _vars.value().selected == ctx.serial()
        || _vars.value().provisionally_selected == ctx.serial();
}

void Selectable::setSelected(SelectableContext& ctx, bool state)
{
    _vars.next().selected = state
        ? ctx.serial()
        : ctx.serial() - 1;
}

void Selectable::setProvisionallySelected(SelectableContext& ctx, bool state)
{
    _vars.next().provisionally_selected = state
        ? ctx.serial()
        : ctx.serial() - 1;
}

void Selectable::prepareNextFrame(SelectableContext& ctx)
{
    _is_activated = _vars.value().selected != ctx.serial()
        && _vars.next().selected == ctx.serial();
    _vars.swap();
    this->setProvisionallySelected(ctx, false);
}

void Selectable::update(
    SelectableContext& ctx,
    bool is_ctrl_held_down,
    bool is_clicked,
    bool is_released,
    bool is_dragged)
{
    this->updateSerials(ctx);
    if (is_clicked) {
        this->on_click(ctx, is_ctrl_held_down);
    }
    if (is_released) {
        this->on_release(ctx, is_ctrl_held_down);
    }
    if (is_dragged) {
        this->on_drag(ctx);
    }
}

void Selectable::on_click(SelectableContext& ctx, bool is_ctrl_held_down)
{
    if (!this->isSelected(ctx) && !is_ctrl_held_down) {
        ctx.deselectAll();
    }

    _wants_deselect = _vars.value().selected == ctx.serial();
    setSelected(ctx, true);
}

void Selectable::on_release(SelectableContext& ctx, bool is_ctrl_held_down)
{
    if (_wants_deselect) {
        setSelected(ctx, false);
        if (!is_ctrl_held_down) {
            ctx.deselectAll();
        }
    }
}

void Selectable::on_drag(SelectableContext& ctx)
{
    _wants_deselect = false;
}

void Selectable::updateSerials(SelectableContext& ctx)
{
    auto serial = ctx.serial();

    _vars.next().selected = _vars.next().selected == serial
        ? serial
        : serial - 1;
    _vars.next().provisionally_selected = _vars.next().provisionally_selected == serial
        ? serial
        : serial - 1;
}

} // namespace ImFlow::detail
