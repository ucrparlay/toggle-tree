#pragma once
#include "parallel_bitmap.h"

struct Frontier {
    ParallelBitmap<true> frontier;
    ParallelBitmap<true> next;
    Frontier(size_t n, uint64_t fork_depth = 5): frontier(n, false, fork_depth), next(n, false, fork_depth) {}
    inline bool exist_frontier() { return frontier.exist_true(); }
    inline void set_fork_depth(int depth) { frontier.set_fork_depth(depth); next.set_fork_depth(depth); }
    inline bool is_frontier(size_t i) { return next.is_true(i); }
    inline void insert(size_t i) { next.mark_true(i); }
    inline bool round_switch() { std::swap(frontier, next); return exist_frontier(); }

    template <class F>
    void parallel_do(F&& f) { frontier.parallel_do(f); }
    template <class Graph, class Source, class Cond, class Update>
    void EdgeMapSparse(Graph& G, Source&& source, Cond&& cond, Update&& update) { frontier.EdgeMapSparse(G, source, cond, update); }

    template<class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){ return frontier.reduce(f, combine); }
    inline uint64_t reduce_max(parlay::sequence<uint32_t>& array){ return frontier.reduce_max(array); }
    inline uint64_t reduce_min(parlay::sequence<uint32_t>& array){ return frontier.reduce_min(array); }
    inline uint64_t reduce_sum(parlay::sequence<uint32_t>& array){ return frontier.reduce_sum(array); }
    inline uint64_t reduce_vertex(){ return frontier.reduce_vertex(); }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){ return frontier.reduce_edge(G); }

    void merge_to(ParallelBitmap<false>& active) { frontier.merge_to(active); }
};