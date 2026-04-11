#pragma once
#include <toggle/toggle.h>
#include <vector>

// Returns (coreness result, per-frontier-round times in seconds).
// Timing covers advance_to_next + for_each for each inner round,
// matching the granularity of Kcore_Hashbag_round.
template <class Graph>
std::pair<parlay::sequence<uint32_t>, std::vector<double>>
KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);
    auto rounds = std::vector<double>();

    auto active = toggle::Active(n);
    auto frontier = toggle::Frontier(n);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.remove(s); }
        return G.offsets[s+1] - G.offsets[s];
    });

    parlay::internal::timer t;
    while (!active.empty()) {
        uint32_t k = active.reduce_min(D);
        active.for_each([&](uint32_t s) {
            if (D[s] == k) { active.remove(s); frontier.insert_next(s); }
        });
        while (true) {
            t.start();
            if (!frontier.advance_to_next()) { t.stop(); break; }
            frontier.for_each([&](uint32_t s) {
                result[s] = k;
                parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                    uint32_t d = G.edges[i].idx;
                    if (active.contains(d)) {
                        if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                            active.remove(d); frontier.insert_next(d);
                        }
                    }
                }, 256);
            });
            rounds.push_back(t.stop());
        }
    }

    return {result, rounds};
}
