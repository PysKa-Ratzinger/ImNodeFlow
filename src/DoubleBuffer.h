#pragma once

namespace ImFlow::detail {

template <typename T>
class DoubleBuffer {
public:
    DoubleBuffer() = default;

    explicit DoubleBuffer(T initial_value)
        : _value(initial_value)
        , _next(initial_value)
    {
    }

    T const& value() const
    {
        return _value;
    }

    T& next()
    {
        return _next;
    }

    void emplace_next(T next_value)
    {
        _next = next_value;
    }

    void swap()
    {
        _value = _next;
    }

private:
    T _value;
    T _next;
};

} // namespace ImFlow::detail
