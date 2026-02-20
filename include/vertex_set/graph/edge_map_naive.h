#pragma once
#include "graph.h"

template <class Graph, class Cond, class Update>
void EdgeMap(Graph& G, parlay::sequence<uint32_t> frontier, Cond&& cond, Update&& update){
    parlay::parallel_for(0, frontier.size(), [&](size_t i) {
        uint32_t s = frontier[i];
        auto start = G.offsets[s];
        auto end   = G.offsets[s + 1];
        for (size_t e = start; e < end; ++e) {
            uint32_t d = G.edges[e].v;
            if (cond(d)) update(d);
        }
    });
}