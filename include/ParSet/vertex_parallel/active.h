#pragma once
#include "parallel_bitmap.h"

namespace ParSet {

struct Active {
    internal::ParallelBitmap active; 
    Active(size_t n, uint64_t fork_depth = 4) : active(n, true, fork_depth) {}
    Active(size_t n, bool init, uint64_t fork_depth) : active(n, init, fork_depth) {}
    inline bool empty() const noexcept { return active.empty(); }
    inline bool is_active(size_t i) const noexcept { return active.is_true(i); }
    inline void activate(size_t i) noexcept { active.set(i); }
    inline void deactivate(size_t i) noexcept { active.clear(i); }
    inline bool try_activate(size_t i) noexcept { return active.try_set(i); }
    inline bool try_deactivate(size_t i) noexcept { return active.try_clear(i); }
    inline void coarse_granularity() noexcept { active.set_fork_depth(4); }
    inline void fine_granularity() noexcept { active.set_fork_depth(5); }

    template <bool Remove = false, class F>
    void parallel_do(F&& f) { active.parallel_do<Remove>(f); }

    template<bool Write = false, class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return active.reduce<Write>(f, combine); }

    template <class F, class Select>
    inline void select(F&& f, Select&& sel) { active.select(f, sel); }

    template<bool Remove = true>
    inline parlay::sequence<uint32_t> pack() { return active.pack<Remove>(); }
    template<bool Remove = true, class Sequence>
    inline size_t pack(Sequence& out) { return active.pack<Remove>(out); }

    template<class Array>
    inline uint64_t reduce_max(Array& array){ 
        return reduce(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (r == 0 || l > r) ? l : r; }
        );
    }
    template<class Array>
    inline uint64_t reduce_min(Array& array){ 
        return reduce(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
        );
    }
    template<class Array>
    inline uint64_t reduce_sum(Array& array){ 
        return reduce(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    inline uint64_t reduce_vertex(){ 
        return reduce(
            [&] (size_t i) { return 1; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){ 
        return reduce(
            [&] (size_t i) { return G.offsets[i+1] - G.offsets[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }

    template<class Array, class Select>
    inline uint64_t reduce_and_select_max(Array& array, Select&& sel){ 
        uint64_t max_value = active.reduce<true>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (r == 0 || l > r) ? l : r; }
        );
        select([&] (size_t i) { return array[i]; }, sel);
        return max_value;
    }
    template<class Array, class Select>
    inline uint64_t reduce_and_select_min(Array& array, Select&& sel){ 
        uint64_t min_value = active.reduce<true>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
        );
        select([&] (size_t i) { return array[i]; }, sel);
        return min_value;
    }

    template<class Frontier, class F>
    inline void pop(uint32_t k, Frontier& frontier, F&& f) { active.pop(k, frontier, f); }
    inline void pop64(uint32_t& k, parlay::sequence<uint32_t>& array) { active.pop64(k, array); }
};

} // namespace ParSet
