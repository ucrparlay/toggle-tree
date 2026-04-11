// SPDX-License-Identifier: MIT
#pragma once

namespace toggle { 

template<class T> inline bool write_min(T& address, T value){
    T old_value = address;
    while (old_value > value && !__atomic_compare_exchange_n(&address, &old_value, value, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old_value > value;
}

template<class T> inline bool write_max(T& address, T value){
    T old_value = address;
    while (old_value < value && !__atomic_compare_exchange_n(&address, &old_value, value, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old_value < value;
}

} // namespace toggle