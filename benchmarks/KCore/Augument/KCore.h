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

    parlay::internal::timer t; double t1=0,t2=0,t3=0;
    while (!active.empty()) {
        // uint32_t k = active.reduce_and_select_min(D, [&](uint32_t s) { active.deactivate(s); frontier.insert(s); });

        t.start();
        uint32_t k = active.reduce<true>(
            [&](size_t s) { return D[s]; },
            [&](uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
        );
        t1 += t.stop();

        t.start();
        active.select(
            [&](size_t s) { return D[s]; },
            [&](uint32_t s) { active.deactivate(s); frontier.insert(s); }
        );
        t2 += t.stop();

        t.start();
        while (frontier.advance()) {
            frontier.parallel_do([&](uint32_t s) { 
                result[s] = k;
                ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t j) {
                    uint32_t d = G.edges[j].v;
                    if (active.is_active(d)) {
                        if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                            active.deactivate(d); frontier.insert(d);
                        }
                    }
                });
            });
        }
        t3 += t.stop();
    }
    std::cerr << "t1 = " << t1 << "\n";
    std::cerr << "t2 = " << t2 << "\n";
    std::cerr << "t3 = " << t3 << "\n";
    
    return result;
}