#include "PinLinkRef.h"

#include "ImNodeFlow/PinConnStrat.h"
#include "ImNodeFlow/PinRef.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/fwd.h"

#include "./link_base.h"
#include "./link_pin.h"
#include "./pin_base.h"

#include "imgui.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>
#include <vector>

namespace ImFlow::detail {

template <bool TIsConst>
PinLinkRef<TIsConst>::PinLinkRef(context_type& ctx, link_type& link)
    : m_ctx(ctx)
    , m_link(link)
{
}

template <bool TIsConst>
PinRefImpl<TIsConst> PinLinkRef<TIsConst>::outPin() const
{
    return PinRefImpl<TIsConst> { m_ctx, *m_link.get().out_pin };
}

template <bool TIsConst>
PinRefImpl<TIsConst> PinLinkRef<TIsConst>::inPin() const
{
    return PinRefImpl<TIsConst> { m_ctx, *m_link.get().in_pin };
}

template <bool TIsConst>
bool PinLinkRef<TIsConst>::isAlive() const
{
    auto& link = m_link.get();
    return !link.base.marked_for_deletion
        && link.in_pin != nullptr
        && link.out_pin != nullptr;
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::connect(PinRef& out_pin, PinRef& in_pin)
    requires(!TIsConst)
{
    this->destroy();

    link_pin_t& link = m_link.get();
    link.base.marked_for_deletion = false;
    link.out_pin = &static_cast<pin_base_t&>(out_pin);
    link.in_pin = &static_cast<pin_base_t&>(in_pin);

    assert(link.out_pin != link.in_pin
        && "Links cannot be created starting and ending in the same node");
    assert(link.out_pin->type != link.in_pin->type
        && "Links cannot be created between pins of the same type");

    if (link.out_pin->type == PinType::Input) {
        std::swap(link.out_pin, link.in_pin);
    }

    if (link.out_pin->strat == PinConnStrat::AllowOnlySingleConnection) {
        this->outPin().deleteAllLinks();
    }
    link.out_pin->links.emplace_back(&link);

    if (link.in_pin->strat == PinConnStrat::AllowOnlySingleConnection) {
        this->inPin().deleteAllLinks();
    }
    link.in_pin->links.emplace_back(&link);
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::destroy()
    requires(!TIsConst)
{
    auto& link = m_link.get();
    if (link.out_pin != nullptr) {
        auto iter = std::find(
            std::begin(link.out_pin->links),
            std::end(link.out_pin->links),
            &link);
        if (iter != std::end(link.out_pin->links)) {
            link.out_pin->links.erase(iter);
        }
        link.out_pin = nullptr;
    }
    if (link.in_pin != nullptr) {
        auto iter = std::find(
            std::begin(link.in_pin->links),
            std::end(link.in_pin->links),
            &link);
        if (iter != std::end(link.in_pin->links)) {
            link.in_pin->links.erase(iter);
        }
        link.in_pin = nullptr;
    }
    link.base.marked_for_deletion = true;
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::update()
    requires(!TIsConst)
{
    auto& ctx = m_ctx.get();
    detail::link_pin_t& link = m_link.get();

    if (link.base.marked_for_deletion) {
        return;
    }

    float thickness = link.base.hovered
        ? outPin().getStyle().extra.link_hovered_thickness
        : outPin().getStyle().extra.link_thickness;

    auto start_color = outPin().getStyle().color;
    auto end_color = inPin().getStyle().color;

    ImVec2 p1 = outPin().pinPoint();
    ImVec2 p4 = inPin().pinPoint();
    auto p2 = p1 + ImVec2(20.F, 0.F);
    auto p3 = p4 - ImVec2(20.F, 0.F);

    link_base_update(
        ctx,
        link.base,
        link_base_update_args {
            .start_color = start_color,
            .end_color = end_color,
            .start_anchor = p1,
            .start = p2,
            .end = p3,
            .end_anchor = p4,
            .thickness = thickness,
            .drag_everything = inPin().getParent().wantsDrag()
                && outPin().getParent().wantsDrag(),
        });

    if (link.base.marked_for_deletion) {
        this->destroy();
    }
}

template <bool TIsConst>
void PinLinkRef<TIsConst>::prepareNextFrame()
    requires(!TIsConst)
{
    auto& link = m_link.get();
    link_base_prepare_next_frame(m_ctx, link.base);
}

template class PinLinkRef<false>;
template class PinLinkRef<true>;

} // namespace ImFlow::detail
