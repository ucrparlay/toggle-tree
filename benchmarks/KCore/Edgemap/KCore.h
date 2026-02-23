#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);

    auto active = ParSet::Active(n);
    auto frontier = ParSet::Frontier(n);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){ 
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.deactivate(s); }
        return G.offsets[s+1] - G.offsets[s]; 
    });

    while (!active.empty()) {
        uint32_t k = active.reduce_min(D);
        active.parallel_do([&](uint32_t s) { 
            if (D[s] == k) { active.deactivate(s); frontier.insert(s);}
        });
        while (frontier.advance()) {
            frontier.edgemap(G,
                [&](uint32_t s) { result[s] = k; },
                [&](uint32_t d) { return active.is_active(d); },
                [&](uint32_t s, uint32_t d) { 
                    if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                        active.deactivate(d); frontier.insert(d);
                    } 
                }
            );
        }
    }
    
    return result;
}