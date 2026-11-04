#pragma once

namespace ImFlow::detail {

struct link_base_t {
    bool marked_for_deletion = false;
    bool hovered = false;
    bool selected = false;
};

} // namespace ImFlow::detail
