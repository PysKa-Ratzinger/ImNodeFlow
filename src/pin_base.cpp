#include "./pin_base.h"

#include "ImNodeFlow/PinInfo.h"
#include "ImNodeFlow/PinType.h"
#include "ImNodeFlow/detail/fwd.h"
#include "ImNodeFlow/fwd.h"

#include "./uid_helpers.h"

#include <memory>
#include <utility>

namespace ImFlow::detail {

pin_base_t::pin_base_t(
    ImNodeFlow& ctx,
    detail::node_t& parent,
    PinType pin_type,
    BasePinInfo info,
    std::unique_ptr<IPin> impl) noexcept
    : ctx { ctx }
    , uid { detail::generate_pin_uid() }
    , name { std::move(info.name) }
    , type { pin_type }
    , parent { &parent }
    , style { std::move(info.style) }
    , strat { info.strat }
    , allowSelfConnection { info.allow_same_node_connections }
    , renderer { std::move(info.renderer) }
    , underlying_pin { std::move(impl) }
{
}

} // namespace ImFlow::detail
