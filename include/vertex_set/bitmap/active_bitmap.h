#pragma once
#include "parallel_bitmap.h"

struct Active {
    ParallelBitmap<false> active; 
    Active(size_t n, uint64_t fork_depth = 4) : active(n, true, fork_depth) {}
    Active(size_t n, bool init, uint64_t fork_depth) : active(n, init, fork_depth) {}
    inline bool exist_active() { return active.exist_true(); }
    inline void set_fork_depth(int depth) { active.set_fork_depth(depth); }
    inline bool is_active(size_t i) { return active.is_true(i); }
    inline void mark_live(size_t i) { active.mark_true(i); }
    inline void mark_dead(size_t i) { active.mark_false(i); }
    inline bool test_mark_live(size_t i) { return active.test_mark_true(i); }
    inline bool test_mark_dead(size_t i) { return active.test_mark_false(i); }

    template <class F>
    inline void parallel_do(F&& f) { active.parallel_do(f); }
    template <class Graph, class Source, class Cond, class Update>
    void EdgeMapSparse(Graph& G, Source&& source, Cond&& cond, Update&& update) { active.EdgeMapSparse(G, source, cond, update); }

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
