#pragma once
#include <toggle/toggle.h>
#include "hashbag.h"
#include <vector>
#include <utility>

// Returns (coloring result, per-frontier-round data as {frontier_size, time}).
// Initial priority setup is not timed. Timing covers pack_into + parallel
// processing for each round with frontier_size > 0.
template <class Graph>
std::pair<parlay::sequence<uint32_t>, std::vector<std::pair<size_t, double>>>
Coloring(Graph& G) {
    size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX);
    auto rounds = std::vector<std::pair<size_t, double>>();

    auto frontier = hashbag<uint32_t>(n);
    auto frt = parlay::sequence<uint32_t>(n, 0);
    auto perm = parlay::random_permutation<uint32_t>(n);
    auto priorities = parlay::tabulate<uint32_t>(n, [&](size_t s){
        uint32_t deg_s = G.offsets[s+1] - G.offsets[s];
        uint32_t perm_s = perm[s];
        uint32_t cnt = parlay::reduce(
            parlay::delayed_tabulate(G.offsets[s + 1] - G.offsets[s], [&](size_t i) -> uint32_t {
                uint32_t d = G.edges[G.offsets[s] + i].idx;
                uint32_t deg_d = G.offsets[d + 1] - G.offsets[d];
                return (deg_d > deg_s) || (deg_d == deg_s && perm[d] < perm_s);
            })
        );
        if (cnt == 0) { frontier.insert(s); }
        return cnt;
    });

    parlay::internal::timer t;
    while (true) {
        t.start();
        auto frt_size = frontier.pack_into(frt);
        if (frt_size == 0) { t.stop(); break; }
        parlay::parallel_for(0, frt_size, [&](uint32_t i) {
            uint32_t s = frt[i];
            uint32_t deg = G.offsets[s + 1] - G.offsets[s];
            if (deg == 0) { result[s] = 0; return; }
            uint8_t* bits = (deg < (1 << 20)) ? (uint8_t*)__builtin_alloca(deg) : (uint8_t*)malloc(deg);
            parlay::parallel_for(0, (deg+(1<<14)-1)>>14, [&](size_t i){
                size_t l = i<<14, r = ((i+1)<<14)<deg?((i+1)<<14):deg;
                __builtin_memset(bits+l, 0, r-l);
            });
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].idx;
                if (result[d] == UINT32_MAX) {
                    if (__atomic_fetch_sub(&priorities[d], 1, __ATOMIC_RELAXED) == 1) { frontier.insert(d); }
                }
                else {
                    if (result[d] < deg) { bits[result[d]] = 1; }
                }
            }, 256);
            uint32_t chosen = UINT32_MAX;
            for (uint32_t i = 0; i < deg; ++i) {
                if (bits[i] == 0) { chosen = i; break; }
            }
            if (deg >= (1 << 20)) free(bits);
            result[s] = (chosen == UINT32_MAX) ? (deg + 1) : chosen;
        });
        rounds.push_back({frt_size, t.stop()});
    }

    return {result, rounds};
}
