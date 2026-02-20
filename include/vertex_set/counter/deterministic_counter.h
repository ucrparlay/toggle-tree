#pragma once
#include <cstdint>
struct Counter {
    uint32_t value;
    Counter(uint32_t value_) : value(value_) {}
    Counter(const Counter& other): value(other.value) { }
    inline void increment() { __atomic_fetch_add(&value, 1, __ATOMIC_RELAXED); }
    //inline bool decrement() { if (value == 0) return false; return __atomic_fetch_sub(&value, 1, __ATOMIC_RELAXED) == 1; }
    inline bool decrement() { return __atomic_fetch_sub(&value, 1, __ATOMIC_RELAXED) == 1; }
    inline bool seq_decrement() { return value-- == 1; }
    //inline bool is_zero() const  noexcept { return value == 0; }
    inline bool is_zero() const   noexcept { return __atomic_load_n(&value, __ATOMIC_RELAXED) == 0; }
    inline bool set_zero()  noexcept { return __atomic_exchange_n(&value, 0, __ATOMIC_ACQ_REL) != 0; }
    inline uint32_t load() const   noexcept { return __atomic_load_n(&value, __ATOMIC_RELAXED); }
    inline uint32_t seq_load() const   noexcept { return __atomic_load_n(&value, __ATOMIC_RELAXED); }
};

#include "parlay/parallel.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"
struct CounterArray {
    parlay::sequence<Counter> counters;

    inline bool decrement(size_t i) { 
        return counters[i].decrement();
    }
    inline bool seq_decrement(size_t i) { 
        return counters[i].seq_decrement();
    }
    
    template <class Graph, class Insert>
    CounterArray(Graph& G, Insert&& insert) { 
        counters = parlay::sequence<Counter>::from_function(G.n, [&](size_t i) {
            uint32_t initial = G.offsets[i + 1] - G.offsets[i];
            if (initial == 0) { insert(i); }
            return Counter(initial);
        });
    }
};
