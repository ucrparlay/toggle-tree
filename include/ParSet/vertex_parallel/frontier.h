#pragma once
#include "parallel_bitmap.h"

namespace ParSet {

struct Frontier {
private:
    internal::ParallelBitmap frontier;
    internal::ParallelBitmap next;
public:
    Frontier(size_t n, uint64_t fork_depth = 5): frontier(n, false, fork_depth), next(n, false, fork_depth) {}
    inline bool empty() const noexcept { return frontier.empty(); }
    inline bool is_frontier(size_t i) const noexcept { return frontier.is_true(i); }
    inline void insert(size_t i) noexcept { next.set(i); }
    inline void remove(size_t i) noexcept { next.clear(i); }
    inline bool try_insert(size_t i) noexcept { return next.try_set(i); }
    inline bool try_remove(size_t i) noexcept { return next.try_clear(i); }
    inline bool advance() noexcept { std::swap(frontier, next); return !empty(); }
    inline void coarse_granularity() noexcept { frontier.set_fork_depth(4); next.set_fork_depth(4); }
    inline void fine_granularity() noexcept { frontier.set_fork_depth(5); next.set_fork_depth(4); }

    template <bool Remove = true, class F>
    void parallel_do(F&& f) { frontier.parallel_do<Remove>(f); }

    template<bool Write = false, class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return frontier.reduce<Write>(f, combine); }

    template <class F, class Select>
    inline void select(F&& f, Select&& sel) { frontier.select(f, sel); }

    template<bool Remove = true>
    inline parlay::sequence<uint32_t> pack() { return frontier.pack<Remove>(); }
    template<bool Remove = true, class Sequence>
    inline size_t pack(Sequence& out) { return frontier.pack<Remove>(out); }

    void merge_to(internal::ParallelBitmap& active) { frontier.merge_to(active); }


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
        uint64_t max_value = frontier.reduce<true>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (r == 0 || l > r) ? l : r; }
        );
        select([&] (size_t i) { return array[i]; }, sel);
        return max_value;
    }
    template<class Array, class Select>
    inline uint64_t reduce_and_select_min(Array& array, Select&& sel){ 
        uint64_t min_value = frontier.reduce<true>(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
        );
        select([&] (size_t i) { return array[i]; }, sel);
        return min_value;
    }

    
};

} // namespace ParSet