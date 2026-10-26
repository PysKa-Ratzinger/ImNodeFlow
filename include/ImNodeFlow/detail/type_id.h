#ifndef IMNODEFLOW_DETAIL_TYPEID_H
#define IMNODEFLOW_DETAIL_TYPEID_H
#pragma once

namespace ImFlow::detail {

inline uint64_t
next_type_id()
{
    static uint64_t id = 0;
    return ++id;
}

template <typename T>
inline uint64_t type_id = next_type_id();

/**
 * @brief Returns a locally unique identifier for a specified type
 *
 * This allows pins to connect only to pins of the same type
 */
template <typename T>
inline static uint64_t get_type_id()
{
    return type_id<T>;
}

} // namespace ImFlow::detail

#endif // IMNODEFLOW_DETAIL_TYPEID_H
