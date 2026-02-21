#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<bool> MIS(Graph& G) {
    const size_t n = G.n;
    auto perm = parlay::random_permutation<uint32_t>(n);
    auto active = ParSet::Active(n); 
    auto in_mis = ParSet::Bitmap(n);
    auto frontier = ParSet::Frontier(n);

    parlay::internal::timer t;
    t.start();
    auto priorities = parlay::tabulate<uint32_t>(n, [&](size_t s){
        uint32_t perm_s = perm[s];
        uint32_t cnt = ParSet::adaptive_sum(G, s, G.offsets[s], G.offsets[s+1],
            [&](uint32_t d) { return perm[d] < perm_s; }
        );
        if (cnt == 0) {
            active.deactivate(s);
            in_mis.seq_set(s);
            if (G.offsets[s+1] - G.offsets[s] > 0) frontier.insert(s);
        }
        return cnt;
    });
    std::cerr << "init: " << t.stop() << "\n";

    while (!active.empty()) {
        frontier.advance();
        frontier.parallel_do([&](size_t s) {
            ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                uint32_t d = G.edges[i].v;
                if (active.try_deactivate(d)) frontier.insert(d);
            });
        });
        frontier.advance();
        frontier.parallel_do([&](size_t s) {
            uint32_t perm_s = perm[s];
            ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                uint32_t d = G.edges[i].v;
                if (active.is_active(d) && perm_s < perm[d]) {
                    if (__atomic_fetch_sub(&priorities[d], 1, __ATOMIC_RELAXED) == 1) {
                        active.deactivate(d);
                        in_mis.set(d);
                        frontier.insert(d);
                    }
                }
            });
        });
    }
    auto result = parlay::sequence<bool>(n, false);
    in_mis.parallel_do([&](size_t s) { result[s] = true; });
    return result;
}
