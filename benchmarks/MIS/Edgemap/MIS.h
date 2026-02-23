#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<bool> MIS(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<bool>(n, false);

    auto perm = parlay::random_permutation<uint32_t>(n);
    auto active = ParSet::Active(n); 
    auto frontier = ParSet::Frontier(n);
    auto priorities = parlay::tabulate<uint32_t>(n, [&](size_t s){
        uint32_t perm_s = perm[s];
        uint32_t cnt = ParSet::adaptive_sum(G, s, G.offsets[s], G.offsets[s+1],
            [&](uint32_t d) { return perm[d] < perm_s; }
        );
        if (cnt == 0) {
            active.deactivate(s);
            result[s] = true;
            if (G.offsets[s+1] - G.offsets[s] > 0) frontier.insert(s);
        }
        return cnt;
    });

    while (frontier.advance()) {
        frontier.parallel_do([&](size_t s) {
            ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                uint32_t d = G.edges[i].v;
                if (active.try_deactivate(d)) frontier.insert(d);
            });
        });
        frontier.advance();
        frontier.edgemap(G,
            [&] (uint32_t s) { return; },
            [&] (uint32_t d) { return active.is_active(d); },
            [&] (uint32_t s, uint32_t d) { 
                if (perm[s] < perm[d] && __atomic_fetch_sub(&priorities[d], 1, __ATOMIC_RELAXED) == 1){
                    active.deactivate(d);
                    result[d] = true;
                    frontier.insert(d);
                }
            }
        );
    }
    
    return result;
}
