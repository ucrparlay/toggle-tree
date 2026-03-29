#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<int32_t> WeightedBFS(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX);
    auto frontier = ParSet::Frontier(n);
    auto tree = ParSet::internal::TournamentTree(dist); 
    tree.update(source, 0); 
    parlay::internal::timer t; double t1=0,t2=0,t3=0;
    for (uint32_t round = 0;  ;round++) {
        int32_t min_value = tree.extract([&] (size_t s) { frontier.insert_next(s); });
        if (min_value == INT32_MAX) break;
        frontier.advance_to_next();
        frontier.for_each([&](uint32_t s) { 
            int32_t dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].v;
                int32_t w = G.edges[i].w;
                if (dist[d] > dist_s + w) {
                    tree.update(d, dist_s + w);
                }
            }, 256);
        });
    }
    return dist;
}