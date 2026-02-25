#pragma once
#include "parallel_bitmap.h"

namespace ParSet {

struct Active {
    internal::ParallelBitmap active; 
    Active(size_t n, bool init = true, uint64_t fork_depth = 4) : active(n, init, fork_depth) {}

    inline bool empty() const noexcept { return active.empty(); }
    inline bool contains(size_t i) const noexcept { return active.contains(i); }
    inline void insert(size_t i) noexcept { active.insert(i); }
    inline void remove(size_t i) noexcept { active.remove(i); }
    inline bool try_insert(size_t i) noexcept { return active.try_insert(i); }
    inline bool try_remove(size_t i) noexcept { return active.try_remove(i); }

    template <bool Remove = false, class F>
    void for_each(F&& f) { active.for_each<Remove>(f); }

    template<bool Write = false, class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return active.reduce<Write>(f, combine); }

    template <class F, class Select>
    inline void select(F&& f, Select&& sel) { active.select(f, sel); }

    template<bool Remove = true>
    inline parlay::sequence<uint32_t> pack() { return active.pack<Remove>(); }
    template<bool Remove = true, class Sequence>
    inline size_t pack_into(Sequence& out) { return active.pack_into<Remove>(out); }

    template<class F>
    inline void pop(uint32_t k, F&& f) { active.pop(k, f); }

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
};

} // namespace ParSet
