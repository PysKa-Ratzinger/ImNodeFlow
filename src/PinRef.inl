#include "./ImNodeFlow_internals.h"

namespace ImFlow {

template <bool TIsConst>
template <typename T>
InPinRef<T, TIsConst>
PinRef<TIsConst>::toIn()
{
    return InPinRef<T, TIsConst> { m_pin.get().template toIn<T>() };
}

template <bool TIsConst>
template <typename T>
OutPinRef<T, TIsConst>
PinRef<TIsConst>::toOut()
{
    return OutPinRef<T, TIsConst> { m_pin.get().template toOut<T>() };
}

// -----------------------------------------------------------------------------------------------------------------
// IN PIN

template <typename T, bool TIsConst>
const T&
InPinRef<T, TIsConst>::value() const
{
    return m_in_pin.get().val;
}

// -----------------------------------------------------------------------------------------------------------------
// OUT PIN

template <typename T, bool TIsConst>
const T&
OutPinRef<T, TIsConst>::value() const
{
    return m_out_pin.get().val;
}

template <typename T, bool TIsConst>
OutPinRef<T, TIsConst>& OutPinRef<T, TIsConst>::behaviour(out_pin_behaviour_t<T> func)
    requires(!TIsConst)
{
    auto& pin = m_out_pin.get();
    pin.behaviour = std::move(func);
    return *this;
}

} // namespace ImFlow
