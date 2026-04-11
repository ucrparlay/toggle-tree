#pragma once
#include <cmath>
#include <toggle/toggle.h>

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);
    auto active = toggle::Active(n);
    auto frontier = toggle::Frontier(n);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){ 
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.remove(s); }
        return G.offsets[s+1] - G.offsets[s]; 
    });
    parlay::internal::timer t; double t0=0, t1=0;
    uint32_t k = 0; int32_t use_reduce = -2*std::log2(n);
    while (!active.empty()) {
        t.start();
        k = use_reduce>=0 ? active.reduce_min(D) : k+1;
        t0 += t.stop();
        t.start();
        active.for_each([&](uint32_t s) { 
            if (D[s] == k) {
                active.remove(s);
                frontier.insert_next(s);
            }
        });
        if (use_reduce<0 && frontier.empty_next()) { use_reduce++; }
        t1 += t.stop();
        while (frontier.advance_to_next()) {
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
        }
    }
    std::cerr << "\nt0=" << t0 << "\nt1=" << t1 << "\n";
    return result;
}
