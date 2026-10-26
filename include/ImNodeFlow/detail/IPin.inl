#include "ImNodeFlow/detail/type_id.h"

namespace ImFlow::detail {

template <typename T>
in_pin_t<T>& IPin::toIn()
{
    assert(PinRef<true> { *this }.getType() == PinType::Input
        && get_type_id<T>() == this->getTypeID());
    return static_cast<in_pin_t<T>&>(*this);
}

template <typename T>
in_pin_t<T> const& IPin::toIn() const
{
    assert(PinRef<true> { *this }.getType() == PinType::Input
        && get_type_id<T>() == this->getTypeID());
    return static_cast<in_pin_t<T> const&>(*this);
}

template <typename T>
out_pin_t<T>& IPin::toOut()
{
    assert(PinRef<true> { *this }.getType() == PinType::Output
        && get_type_id<T>() == this->getTypeID());
    return static_cast<out_pin_t<T>&>(*this);
}

template <typename T>
out_pin_t<T> const& IPin::toOut() const
{
    assert(PinRef<true> { *this }.getType() == PinType::Output
        && get_type_id<T>() == this->getTypeID());
    return static_cast<out_pin_t<T> const&>(*this);
}

} // namespace ImFlow::detail
