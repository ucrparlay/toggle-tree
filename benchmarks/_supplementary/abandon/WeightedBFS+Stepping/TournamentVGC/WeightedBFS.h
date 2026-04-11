#pragma once
#include <toggle/toggle.h>

template <class Graph, class TournamentTree>
void edgemap(Graph&G, uint32_t s, parlay::sequence<int32_t>& dist, TournamentTree& tree, uint32_t maxiter) {
    int32_t dist_s = dist[s];
    parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
        uint32_t d = G.edges[i].idx;
        int32_t w = G.edges[i].w;
        if (dist[d] > dist_s + w && tree.update(d, dist_s + w)) {
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
    auto tree = toggle::internal::TournamentTree(dist); 
    tree.update(source, 0); 
    uint32_t rho = 2000000;
    uint32_t mode = 0; double delta = 1; uint32_t lastex=0, thisex = 0;
    for (double round = 0; true; ) {
        if (mode != uint32_t(std::log(n))) {
            int32_t min_value = tree.repair();
            if (min_value == INT32_MAX) break;
            int32_t threshold = min_value;
            thisex = tree.extract_min(threshold, [&] (size_t s) { 
                int32_t dist_s = dist[s];
                parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                    uint32_t d = G.edges[i].idx;
                    int32_t w = G.edges[i].w;
                    if (dist[d] > dist_s + w) {
                        tree.update(d, dist_s + w);
                    }
                }, 256);
            });
            round++;
            if (lastex > std::log(n) && thisex > std::log(n)) {
                if (mode % 2 == 0 && lastex > thisex) { mode++; thisex = 0; }
                else if (mode % 2 == 1 && lastex < thisex) { mode++; thisex = 0; }
            }
            lastex = thisex;
        }
        else {
            int32_t min_value = tree.repair();
            if (min_value == INT32_MAX) break;
            uint64_t extracted = tree.extract_min(min_value+delta-1, [&] (size_t s) { 
                edgemap(G,s,dist,tree,min_value+0.2*delta);
            });
            

            if (extracted > 4ull * rho) delta = std::max(1.0, delta * 0.5);
            else if (extracted > 2ull * rho) delta = std::max(1.0, delta * 0.7);
            else if (extracted < rho / 8) delta += 4;
            else if (extracted < rho / 4) delta += 2;
            else if (extracted < rho / 2) delta += 1;
            else if (extracted > rho) delta = std::max(1.0, delta - 1);
        }
    }
    return dist;
}