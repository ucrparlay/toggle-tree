#pragma once
#include <ParSet/ParSet.h>
#include "hashbag.h"
#include <vector>
#include <utility>

// Returns (coreness result, per-frontier-round data as {frontier_size, time})
// Only inner frontier rounds with frontier_size > 0 are recorded.
// Timing covers pack_into + parallel processing for each round.
template <class Graph>
std::pair<parlay::sequence<uint32_t>, std::vector<std::pair<size_t, double>>>
KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);
    auto rounds = std::vector<std::pair<size_t, double>>();

    auto active = ParSet::Active(n);
    auto frontier = hashbag<uint32_t>(n);
    auto frt = parlay::sequence<uint32_t>(n, 0);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.remove(s); }
        return G.offsets[s+1] - G.offsets[s];
    });

    parlay::internal::timer t;
    while (!active.empty()) {
        uint32_t k = active.reduce_min(D);
        active.for_each([&](uint32_t s) {
            if (D[s] == k) { active.remove(s); frontier.insert(s); }
        });
        while (true) {
            t.start();
            auto frt_size = frontier.pack_into(frt);
            if (frt_size == 0) { t.stop(); break; }
            parlay::parallel_for(0, frt_size, [&](uint32_t i) {
                uint32_t s = frt[i];
                result[s] = k;
                parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t j) {
                    uint32_t d = G.edges[j].v;
                    if (active.contains(d)) {
                        if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                            active.remove(d); frontier.insert(d);
                        }
                    }
                }, 256);
            });
            rounds.push_back({frt_size, t.stop()});
        }
    }

    return {result, rounds};
}
