#pragma once
#include <cstdint>
#include "parlay/parallel.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"
#include "graph/initializer.h"
struct Counter {
    uint32_t data;
    Counter(uint32_t data_) : data(2 *data_) {}
    Counter(const Counter& other): data(other.data) { }
    inline bool decrement() noexcept { return __atomic_fetch_sub(&data, 2, __ATOMIC_RELAXED) == 2; }
    inline bool seq_decrement() noexcept { data -= 2; return data == 0; }
    inline bool is_zero() const noexcept { return data == 0; }
    inline bool is_info() const noexcept { return data & 1; }
    inline uint32_t load() const noexcept { return data >> 1; }
    inline void save(uint32_t i) noexcept { data = (i << 1) | 1; }
};

struct CounterArray {
    parlay::sequence<Counter> counters;
    inline bool is_info(size_t i) noexcept { return counters[i].is_info(); }
    inline bool decrement(size_t i) { return counters[i].decrement();}
    inline bool seq_decrement(size_t i) { return counters[i].seq_decrement();}
    inline uint32_t load(size_t i) noexcept { return counters[i].load(); }
    inline void save(size_t i, uint32_t data) noexcept { counters[i].save(data); }

    template <class Graph, class Insert>
    CounterArray(Graph& G, parlay::sequence<uint32_t>& priority1, parlay::sequence<uint32_t>& priority2, Insert&& insert) { 
        init_from_priority(counters, G, priority1, priority2, insert);
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
/*
struct CounterArray {
    parlay::sequence<uint32_t> counters;
    ModeBitmap info;
    inline bool is_info(size_t i) noexcept { return info.is_mode(i); }
    inline bool decrement(size_t i) { 
        return __atomic_fetch_sub(&counters[i], 1, __ATOMIC_RELAXED) == 1;
    }
    inline bool seq_decrement(size_t i) { 
        return counters[i]-- == 1;
    }
    inline uint32_t load(size_t i) noexcept { return counters[i]; }
    inline void save(size_t i, uint32_t data) noexcept { counters[i] = data; }
    inline parlay::sequence<uint32_t>& dump_result() noexcept { return counters; }

    template <class Graph, class Insert>
    CounterArray(Graph& G, parlay::sequence<uint32_t>& priority1, parlay::sequence<uint32_t>& priority2, Insert&& insert):info(G.n) { 
        init_from_priority(counters, G, priority1, priority2, insert);
    }
};*/