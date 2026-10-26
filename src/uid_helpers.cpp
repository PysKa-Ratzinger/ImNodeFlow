#include "uid_helpers.h"

#include "ImNodeFlow/NodeUID.h"
#include "ImNodeFlow/PinUID.h"

namespace ImFlow::detail {

PinUID generate_pin_uid()
{
    static uint64_t last_id = 0;

    return PinUID { ++last_id };
}

NodeUID generate_node_uid()
{
    static uint64_t last_id = 0;

    return NodeUID { ++last_id };
}

} // namespace ImFlow::detail
