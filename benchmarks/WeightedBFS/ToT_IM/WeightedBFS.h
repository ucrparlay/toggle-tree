#pragma once
#include <toggle/toggle.h>

template <class Graph, class TournamentTree>
void edgemap(Graph&G, uint32_t s, parlay::sequence<int32_t>& dist, TournamentTree& tree, uint32_t maxiter) {
    int32_t dist_s = dist[s];
    parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
        uint32_t d = G.edges[i].idx;
        int32_t w = G.edges[i].wgh;
        if (dist[d] > dist_s + w && tree.dec_val(d, dist_s + w)) {
            if (dist_s + w <= maxiter) {
                edgemap(G, d, dist, tree, maxiter);
            }
        }
    }, 256);
}

template <class Graph>
parlay::sequence<int32_t> WeightedBFS(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX);
    auto frontier = toggle::Frontier(n);
    auto tree = toggle::internal::IndexMap(dist); 
    tree.dec_val(source, 0); 
    for (uint32_t round = 0; ;round++) {
        int32_t min_value = tree.repair();
        if (min_value == INT32_MAX) break;
        int32_t threshold = min_value;
        tree.extract_min(threshold, [&] (size_t s) { 
            int32_t dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].idx;
                int32_t w = G.edges[i].wgh;
                if (dist[d] > dist_s + w) {
                    tree.dec_val(d, dist_s + w);
                }
            }, 256);
        });
    }
    return dist;
}