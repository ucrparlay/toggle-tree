#pragma once
#include <ParSet/ParSet.h>

template <class Graph, class TournamentTree>
void edgemap(Graph&G, uint32_t s, parlay::sequence<int32_t>& dist, TournamentTree& tree, uint32_t maxiter) {
    int32_t dist_s = dist[s];
    parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
        uint32_t d = G.edges[i].v;
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
    auto frontier = ParSet::Frontier(n);
    auto tree = ParSet::internal::TournamentTree(dist); 
    tree.update(source, 0); 
    parlay::internal::timer t; double t1=0,t2=0;
    for (uint32_t round = 0; ;round++) {
        t.start();
        int32_t min_value = tree.repair();
        t1+= t.stop();
        if (min_value == INT32_MAX) break;
        int32_t threshold = min_value + 80;
        t.start();
        tree.extract_min(threshold, [&] (size_t s) { 
            edgemap(G,s,dist,tree,threshold);
        });
        t2+= t.stop();
    }
    std::cerr << "\nt1=" << t1 << "  t2=" << t2 << "\n";
    return dist;
}