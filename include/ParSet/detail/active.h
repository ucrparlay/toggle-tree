// SPDX-License-Identifier: MIT
#pragma once
#include "internal/parallel_bitmap.h"

namespace ParSet {

struct Active {
    internal::ParallelBitmap active; 
    uint64_t last_min;
    uint64_t cur_min;
    Active(size_t n, bool init = true) : active(n, init), last_min(0), cur_min(0) {}

    inline bool empty() const noexcept { return active.empty(); }
    inline bool contains(size_t i) const noexcept { return active.contains(i); }
    inline void insert(size_t i) noexcept { active.insert(i); }
    inline void remove(size_t i) noexcept { active.remove(i); }

    template <bool Remove = false, class F>
    void for_each(F&& f) { active.for_each<Remove, 64>(f); }

    template<class T, auto Identity, class F, class Combine>
    inline T reduce(F&& f, Combine&& combine){ return active.reduce<T, Identity>(f, combine); }

    template<class T>
    inline T reduce_min(parlay::sequence<T>& seq){ 
        return reduce<T, std::numeric_limits<T>::max()>(
            [&] (size_t i) { return seq[i]; },
            [&] (T l, T r) { return (l > r) ? r : l; }
        );
    }
    //inline uint64_t reduce_vertex(){ return active.reduce_vertex<false>(); }
    /*
    template<class Array>
    inline uint64_t reduce_max(Array& array){ 
        return reduce<(uint64_t)0>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l > r) ? l : r; }
        );
    }
    template<class Array>
    inline uint64_t reduce_sum(Array& array){ 
        return reduce<(uint64_t)0>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){ 
        return reduce<(uint64_t)0>(
            [&] (size_t i) { return G.offsets[i+1] - G.offsets[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }*/
};

} // namespace ParSet
