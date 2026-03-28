#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);
    auto active = ParSet::Active(n);
    auto frontier = ParSet::Frontier(n);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s){ 
        if (G.offsets[s+1] - G.offsets[s] == 0) { active.remove(s); }
        return G.offsets[s+1] - G.offsets[s]; 
    });

    while (!active.empty()) {
        uint32_t k = active.reduce_min(D);
        active.for_each([&](uint32_t s) { 
            if (D[s] == k) { active.remove(s); frontier.insert_next(s);}
        });
        
        while (frontier.advance_to_next()) {
            if (k < 30) {
                frontier.for_each([&](uint32_t s) { 
                    result[s] = k;
                    ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t i1) { uint32_t s1 = G.edges[i1].v;
                        if (active.contains(s1) && __atomic_fetch_sub(&D[s1], 1, __ATOMIC_RELAXED) == k+1) {
                            active.remove(s1); result[s1] = k;
                            if (G.offsets[s1 + 1] - G.offsets[s1] > 128) { frontier.insert_next(s1); return; }
                            ParSet::adaptive_for(G.offsets[s1], G.offsets[s1 + 1], [&](size_t i2) { uint32_t s2 = G.edges[i2].v;
                                if (active.contains(s2) && __atomic_fetch_sub(&D[s2], 1, __ATOMIC_RELAXED) == k+1) {
                                    active.remove(s2); result[s2] = k;
                                    if (G.offsets[s2 + 1] - G.offsets[s2] > 128) { frontier.insert_next(s2); return; }
                                    ParSet::adaptive_for(G.offsets[s2], G.offsets[s2 + 1], [&](size_t i3) { uint32_t s3 = G.edges[i3].v;
                                        if (active.contains(s3) && __atomic_fetch_sub(&D[s3], 1, __ATOMIC_RELAXED) == k+1) {
                                            active.remove(s3); result[s3] = k;
                                            frontier.insert_next(s3);
                                        }
                                    });
                                }
                            });
                        }
                    });
                });
            }
            else {
                frontier.for_each([&](uint32_t s) { 
                    result[s] = k;
                    ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                        uint32_t d = G.edges[i].v;
                        if (active.contains(d)) {
                            if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k+1) {
                                active.remove(d); frontier.insert_next(d);
                            }
                        }
                    });
                });
            }
            
        }
    }
    
    return result;
}