#pragma once
#include "../hashbag/hashbag.h"

struct Frontier {
    hashbag<uint32_t> bag;
    parlay::sequence<uint32_t> frontier;
    Frontier(size_t n, uint64_t _ = 0): bag(n) {}
    inline bool exist_frontier() { return frontier.size(); }
    inline void set_fork_depth(int depth) { return; }
    inline bool round_switch() { frontier = bag.pack(); return exist_frontier(); }
    inline uint64_t size() { return frontier.size(); }
    inline void insert(size_t i) { bag.insert(i); }
    template <class F>
    void parallel_do(F&& f) { 
        parlay::parallel_for(0, frontier.size(), [&](size_t i){ f(frontier[i]); });
    }
    template <class Graph, class Source, class Cond, class Update>
    void EdgeMapSparse(Graph& G, Source&& source, Cond&& cond, Update&& update) {
        parlay::parallel_for(0, frontier.size(), [&](size_t _) { 
            uint32_t s = frontier[_]; 
            source(s);
            size_t start = G.offsets[s];
            size_t end = G.offsets[s + 1];
            adaptive_for(start, end, [&] (size_t j) {
                uint32_t d = G.edges[j].v;
                if (cond(d)) {
                    update(d);
                }
            });
        });
    }
};