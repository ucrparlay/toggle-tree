#pragma once
#include <toggle/toggle.h>

static inline bool writemin(int& ref, int v){
    int old = ref;
    while (old > v && !__atomic_compare_exchange_n(&ref, &old, v, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old > v;
}

template <class Graph>
parlay::sequence<int32_t> BellmanFord(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto dist = parlay::sequence<int>(n, INT32_MAX); 
    auto frontier = toggle::Frontier(n);
    dist[source] = 0; frontier.insert_next(source);

    for (uint32_t round = 0; round < n && frontier.advance_to_next(); round++) {
        frontier.for_each([&](uint32_t s) { 
            int dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].idx;
                int w = G.edges[i].wgh;
                if (dist[d] > dist_s + w && writemin(dist[d], dist_s + w)) {
                    frontier.insert_next(d);
                }
            }, 256);
        });
    }
    
    return dist;
}