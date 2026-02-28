#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);

    auto active = ParSet::Active(n);
    auto frontier = ParSet::Frontier(n);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){ 
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.remove(s); }
        return G.offsets[s+1] - G.offsets[s]; 
    });

    while (!active.empty()) {
        uint32_t k = active.reduce_min(D);
        active.for_each([&](uint32_t s) { 
            if (D[s] == k) { active.remove(s); frontier.insert_next(s);}
        });
        while (frontier.advance_to_next()) {
            frontier.for_each([&](uint32_t s) { 
                result[s] = k;
                ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                    uint32_t d = G.edges[i].v;
                    if (active.contains(d)) {
                        if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                            active.remove(d); frontier.insert_next(d);
                        }
                    }
                });
            });
        }
    }
    
    return result;
}