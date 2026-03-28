#pragma once
#include <ParSet/ParSet.h>
#include "hashbag.h"

static inline bool writemin(int& ref, int v){
    int old = ref;
    while (old > v && !__atomic_compare_exchange_n(&ref, &old, v, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old > v;
}

static inline bool writeifnot(uint32_t& ref, uint32_t v){
    uint32_t old = ref;
    while (old != v && !__atomic_compare_exchange_n(&ref, &old, v, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old != v;
}

template <class Graph>
parlay::sequence<int32_t> BellmanFord(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX);
    auto frontier = hashbag<uint32_t>(n);
    auto frt = parlay::sequence<uint32_t>(n, 0);
    auto in_next = parlay::sequence<uint32_t>(n, UINT32_MAX);
    dist[source] = 0; frontier.insert(source);

    for (uint32_t round = 0; round < n; round++) {
        auto frt_size = frontier.pack_into(frt);
        if (!frt_size) break;
        parlay::parallel_for(0, frt_size, [&](uint32_t i) {
            uint32_t s = frt[i];
            int dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].v;
                int nd = dist_s + G.edges[i].w;
                if (dist[d] > nd && writemin(dist[d], nd) && writeifnot(in_next[d], round)) frontier.insert(d);
            }, 256);
        });
    }
    return dist;
}