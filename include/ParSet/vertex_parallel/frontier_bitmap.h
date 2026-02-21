#pragma once
#include "parallel_bitmap.h"

namespace ParSet {

struct Frontier {
private:
    internal::ParallelBitmap<true> frontier;
    internal::ParallelBitmap<true> next;
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

    template <class F>
    void parallel_do(F&& f) { frontier.parallel_do(f); }

    template<class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return frontier.reduce(f, combine); }
    inline uint64_t reduce_max(parlay::sequence<uint32_t>& array){ return frontier.reduce_max(array); }
    inline uint64_t reduce_min(parlay::sequence<uint32_t>& array){ return frontier.reduce_min(array); }
    inline uint64_t reduce_sum(parlay::sequence<uint32_t>& array){ return frontier.reduce_sum(array); }
    inline uint64_t reduce_vertex(){ return frontier.reduce_vertex(); }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){ return frontier.reduce_edge(G); }

    void merge_to(internal::ParallelBitmap<false>& active) { frontier.merge_to(active); }
};

} // namespace ParSet