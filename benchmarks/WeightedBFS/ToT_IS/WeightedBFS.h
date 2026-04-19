#pragma once
#include <toggle/toggle.h>

template <class Graph>
parlay::sequence<int32_t> WeightedBFS(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto active = toggle::Active(n, false); 
    auto frontier = toggle::Frontier(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    active.insert(source);
    for (uint32_t round = 0; !active.empty(); round++) {
        active.for_each([&](uint32_t s) { 
            if (dist[s] == round) { active.remove(s); frontier.insert_next(s);}
        });
        frontier.advance_to_next();
        frontier.for_each([&](uint32_t s) { 
            int32_t dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].idx;
                int32_t w = G.edges[i].wgh;
                if (dist[d] > dist_s + w && toggle::write_min(dist[d], dist_s + w)) {
                    active.insert(d);
                }
            }, 256);
        });
    }
    return dist;
}