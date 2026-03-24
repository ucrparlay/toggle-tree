#pragma once
#include "parallel_bitmap.h"

namespace ParSet {

struct Frontier {
    internal::ParallelBitmap frontier;
    internal::ParallelBitmap next;
    Frontier(size_t n): frontier(n, false), next(n, false) {}

    inline bool empty() const noexcept { return frontier.empty(); }
    inline bool contains(size_t i) const noexcept { return frontier.contains(i); }
    inline bool contains_next(size_t i) const noexcept { return next.contains(i); }
    inline void insert_next(size_t i) noexcept { next.insert(i); }
    inline void remove_next(size_t i) noexcept { next.remove(i); }
    inline bool try_insert_next(size_t i) noexcept { return next.try_insert(i); }
    inline bool try_remove_next(size_t i) noexcept { return next.try_remove(i); }
    inline bool advance_to_next() noexcept { std::swap(frontier, next); return !empty(); }
    
    template <bool Remove = true, size_t Gran = 8, class F>
    void for_each(F&& f) { frontier.for_each<Remove, Gran>(f); }

    template<bool Write = false, uint64_t Identity = 0, class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return frontier.reduce<Write, Identity>(f, combine); }
    

    template<bool Remove = true, class T = uint32_t>
    inline parlay::sequence<T> pack() { return frontier.pack<Remove, T>(); }
    template<bool Remove = true, class Sequence>
    inline size_t pack_into(Sequence& out) { return frontier.pack_into<Remove>(out); }

    template<class F>
    inline void pop(size_t k, F&& f) { frontier.pop(k, f); }

    inline uint64_t reduce_vertex(){ return frontier.reduce_vertex<false>(); }
    template<class Array>
    inline uint64_t reduce_min(Array& array){ 
        return reduce<false, UINT64_MAX>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l > r) ? r : l; }
        );
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

    template<class Array, class Act>
    inline void repair(Act& active,Array& dist){
        frontier.repair(active.active, dist);
    }
};

} // namespace ParSet