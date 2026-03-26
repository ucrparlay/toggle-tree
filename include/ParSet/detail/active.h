// SPDX-License-Identifier: MIT
#pragma once
#include "internal/parallel_bitmap.h"

namespace ParSet {

struct Active {
    internal::ParallelBitmap active; 
    Active(size_t n, bool init = true) : active(n, init) {}

    inline bool empty() const noexcept { return active.empty(); }
    inline bool contains(size_t i) const noexcept { return active.contains(i); }
    inline void insert(size_t i) noexcept { active.insert(i); }
    inline void remove(size_t i) noexcept { active.remove(i); }

    template <bool Remove = false, size_t Gran = 64, class F>
    void for_each(F&& f) { active.for_each<Remove, Gran>(f); }

    template<bool Write = false, uint64_t Identity = 0, class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return active.reduce<Write, Identity>(f, combine); }

    template<bool Remove = true, class T = uint32_t>
    inline parlay::sequence<T> pack() { return active.pack<Remove, T>(); }
    template<bool Remove = true, class Sequence>
    inline size_t pack_into(Sequence& out) { return active.pack_into<Remove>(out); }

    template<class F>
    inline void pop(size_t k, F&& f) { active.pop(k, f); }

    inline uint64_t reduce_vertex(){ return active.reduce_vertex<false>(); }
    template<class Array>
    inline uint64_t reduce_min(Array& array){ 
        return reduce<false, UINT64_MAX>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l > r) ? r : l; }
        );
    }
    template<class Array, class Frontier>
    inline uint64_t adaptive_reduce_min(Array& array, Frontier& frontier){ 
        static uint64_t last_min == UINT64_MAX;
        static uint64_t cur_min == UINT64_MAX;
        if (cur_min - last_min != 1) {
            last_min = cur_min; 
        }
        else {
            last_min = cur_min; cur_min++;
            for_each([&] (size_t s) { if (array[s] == cur_min) frontier.next.insert(s); })
            if (!frontier.next.empty()) return cur_min;
        }
        cur_min = reduce_min(array);
        for_each([&] (size_t s) { if (array[s] == cur_min) frontier.next.insert(s); })
    }
    
    template<class Array>
    inline uint64_t reduce_max(Array& array){ 
        return reduce<false, 0>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l > r) ? l : r; }
        );
    }
    template<class Array>
    inline uint64_t reduce_sum(Array& array){ 
        return reduce<false, 0>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){ 
        return reduce<false, 0>(
            [&] (size_t i) { return G.offsets[i+1] - G.offsets[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }

    template<class Dist, class Frt>
    inline uint64_t extract_min(Frt& out,Dist& dist){
        return active.extract_min(dist,[&](uint32_t s){ out.insert_next(s); });
    }
};

} // namespace ParSet
