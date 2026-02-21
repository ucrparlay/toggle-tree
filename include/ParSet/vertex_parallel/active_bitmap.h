#pragma once
#include "parallel_bitmap.h"

namespace ParSet {

struct Active {
private:
    internal::ParallelBitmap<false> active; 
public:
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

    template <class F>
    inline void parallel_do(F&& f) { active.parallel_do(f); }

    template<class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return active.reduce(f, combine); }
    inline uint64_t reduce_max(parlay::sequence<uint32_t>& array){ return active.reduce_max(array); }
    inline uint64_t reduce_min(parlay::sequence<uint32_t>& array){ return active.reduce_min(array); }
    inline uint64_t reduce_sum(parlay::sequence<uint32_t>& array){ return active.reduce_sum(array); }
    inline uint64_t reduce_vertex(){ return active.reduce_vertex(); }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){ return active.reduce_edge(G); }

    template<class Frontier, class F>
    inline void pop(uint32_t k, Frontier& frontier, F&& f) { active.pop(k, frontier, f); }
    inline void pop64(uint32_t& k, parlay::sequence<uint32_t>& array) { active.pop64(k, array); }
};

} // namespace ParSet
