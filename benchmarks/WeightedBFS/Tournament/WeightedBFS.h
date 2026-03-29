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
    for (uint32_t round = 0; ;round++) {
        int32_t min_value = tree.extract([&] (size_t s) { 
            int32_t dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                tree.update(G.edges[i].v, dist_s + G.edges[i].w);
            }, 256);
        });
        if (min_value == INT32_MAX) break;
    }
    return dist;
}