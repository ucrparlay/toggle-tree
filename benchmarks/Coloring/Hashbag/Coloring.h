#pragma once

#include <ParSet/ParSet.h>
#include "hashbag.h"

template <class Graph>
parlay::sequence<uint32_t> Coloring(Graph& G) {
    size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX);

    hashbag<uint32_t> frontier_a(n), frontier_b(n);
    hashbag<uint32_t>* frontier = &frontier_a;
    hashbag<uint32_t>* next_frontier = &frontier_b;
    auto frontier_vertices = parlay::sequence<uint32_t>::uninitialized(n);
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
        if (cnt == 0) { frontier->insert(s); }
        return cnt;
    });

    while (true) {
        size_t frontier_size = frontier->pack_into(parlay::make_slice(frontier_vertices));
        if (!frontier_size) break;

        parlay::parallel_for(0, frontier_size, [&](size_t i){
            size_t s = frontier_vertices[i];
            uint32_t deg = G.offsets[s + 1] - G.offsets[s];
            if (deg == 0) { result[s] = 0; return; }
            uint8_t* bits = (deg < (1 << 20)) ? (uint8_t*)__builtin_alloca(deg) : (uint8_t*)malloc(deg);
            parlay::parallel_for(0, (deg+(1<<14)-1)>>14, [&](size_t j){
                size_t l = j<<14, r = ((j+1)<<14)<deg?((j+1)<<14):deg;
                __builtin_memset(bits+l, 0, r-l);
            });
            ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t j) {
                uint32_t d = G.edges[j].v;
                if (result[d] == UINT32_MAX) {
                    if (__atomic_fetch_sub(&priorities[d], 1, __ATOMIC_RELAXED) == 1) { next_frontier->insert(d);  }
                }
                else {
                    if (result[d] < deg) { bits[result[d]] = 1; }
                }
            });
            uint32_t chosen = UINT32_MAX;
            for (uint32_t j = 0; j < deg; ++j) {
                if (bits[j] == 0) { chosen = j; break; }
            }
            if(deg >= (1 << 20)) free(bits);
            result[s] = (chosen == UINT32_MAX) ? (deg + 1) : chosen;
        });

        std::swap(frontier, next_frontier);
    }

    return result;
}
