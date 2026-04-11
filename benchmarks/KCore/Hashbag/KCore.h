#pragma once
#include <toggle/toggle.h>
#include "hashbag.h"

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);

    auto active = toggle::Active(n);
    auto frontier = hashbag<uint32_t>(n);
    auto frt = parlay::sequence<uint32_t>(n, 0);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){ 
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.remove(s); }
        return G.offsets[s+1] - G.offsets[s]; 
    });

    uint32_t k = 0; int32_t use_reduce = -2*std::log2(n);
    while (!active.empty()) {
        k = use_reduce>=0 ? active.reduce_min(D) : k+1;
        active.for_each([&](uint32_t s) { 
            if (D[s] == k) { active.remove(s); frontier.insert(s); }
        });
        auto frt_size = frontier.pack_into(frt);
        if (use_reduce<0 && frt_size == 0) { use_reduce++; }
        while (frt_size != 0) {
            parlay::parallel_for(0, frt_size, [&](uint32_t i) {
                uint32_t s = frt[i];
                result[s] = k;
                parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t j) {
                    uint32_t d = G.edges[j].idx;
                    if (active.contains(d)) {
                        if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                            active.remove(d); frontier.insert(d);
                        }
                    }
                }, 256);
            });
            frt_size = frontier.pack_into(frt);
        }
    }
    
    return result;
}