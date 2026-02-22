#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> Coloring(Graph& G) {
    size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX);

    auto frontier = ParSet::Frontier(n);
    auto perm = parlay::random_permutation<uint32_t>(n);
    auto priorities = parlay::tabulate<uint32_t>(n, [&](size_t s){
        uint32_t deg_s = G.offsets[s+1] - G.offsets[s];
        uint32_t perm_s = perm[s];
        uint32_t cnt = ParSet::adaptive_sum(G, s, G.offsets[s], G.offsets[s+1],
            [&](uint32_t d) { 
                uint32_t deg_d = G.offsets[d+1] - G.offsets[d]; 
                return (deg_d > deg_s) || (deg_d == deg_s && perm[d] < perm_s); 
            }
        );
        if (cnt == 0) { frontier.insert(s); }
        return cnt;
    });

    for (uint32_t round = 0; frontier.advance(); round++) {
        frontier.parallel_do([&](size_t s){
            result[s] = round;
            ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t j) {
                uint32_t d = G.edges[j].v;
                if (result[d] == UINT32_MAX) {
                    if (__atomic_fetch_sub(&priorities[d], 1, __ATOMIC_RELAXED) == 1) { frontier.insert(d);  }
                }
            });
        });
    }

    return result;
}