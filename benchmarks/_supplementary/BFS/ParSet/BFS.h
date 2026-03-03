#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s=0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = ParSet::Active(n); 
    auto frontier = ParSet::Frontier(n);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    frontier.insert_next(s);
    active.remove(s);
    for (uint32_t round = 0; frontier.advance_to_next(); round++) {
        frontier.for_each([&](uint32_t s) { 
            result[s] = round;
            ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                uint32_t d = G.edges[i].v;
                if (active.try_remove(d)) { 
                    frontier.insert_next(d);
                }
            });
        });
    }
    return result;
}