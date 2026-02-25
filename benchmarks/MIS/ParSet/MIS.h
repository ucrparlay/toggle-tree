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
            active.remove(s);
            result[s] = true;
            if (G.offsets[s+1] - G.offsets[s] > 0) frontier.insert_next(s);
        }
        return cnt;
    });

    while (frontier.advance_to_next()) {
        frontier.for_each([&](size_t s) {
            ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                uint32_t d = G.edges[i].v;
                if (active.try_remove(d)) frontier.insert_next(d);
            });
        });
        frontier.advance_to_next();
        frontier.for_each([&](size_t s) {
            uint32_t perm_s = perm[s];
            ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                uint32_t d = G.edges[i].v;
                if (active.contains(d) && perm_s < perm[d]) {
                    if (__atomic_fetch_sub(&priorities[d], 1, __ATOMIC_RELAXED) == 1) {
                        active.remove(d);
                        result[d] = true;
                        frontier.insert_next(d);
                    }
                }
            });
        });
    }
    
    return result;
}
