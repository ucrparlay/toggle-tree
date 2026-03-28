#pragma once
#include <ParSet/ParSet.h>

static inline bool write_min(int32_t& ref, int32_t v){
    int32_t old = ref;
    while (old > v && !__atomic_compare_exchange_n(&ref, &old, v, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old > v;
}

template <class Graph>
parlay::sequence<int32_t> WeightedBFS(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto active = ParSet::Active(n, false); 
    auto frontier = ParSet::Frontier(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    active.insert(source); 

    uint32_t cur_min = UINT32_MAX, last_min = UINT32_MAX;
    for (uint32_t round = 0; !active.empty(); round++) {
        if (cur_min - last_min == 1) { 
            active.for_each([&] (size_t s) { if (dist[s] == cur_min + 1) { active.remove(s); frontier.insert_next(s); }}); 
        }
        last_min = cur_min; 
        cur_min = frontier.advance_to_next() ? cur_min++ : active.reduce_min(dist);
        if (frontier.empty()) { 
            active.for_each([&] (size_t s) { if (dist[s] == cur_min) { active.remove(s); frontier.insert_next(s); }});
            frontier.advance_to_next();
        }
        frontier.for_each([&](uint32_t s) { 
            int32_t dist_s = dist[s];
            ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].v;
                int32_t w = G.edges[i].w;
                if (dist[d] > dist_s + w && write_min(dist[d], dist_s + w)) {
                    active.insert(d);
                }
            });
        });
    }
    return dist;
}