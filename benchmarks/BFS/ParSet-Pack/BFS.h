#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s=0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = ParSet::Active(n); 
    auto frontier = ParSet::Frontier(n);
    auto frt_seq = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    frontier.insert(s);
    active.deactivate(s);
    for (uint32_t round = 0; ; round++) {
        if (mode == 0) {
            if (!frontier.advance()) break;
            if (round < uint32_t(2 * std::log(n)) && frontier.reduce_edge(G) > 0.1 * G.m) {
                frontier.parallel_do([&](uint32_t s) { result[s] = round; });
                mode = 1; continue;
            }
            size_t frontier_size = frontier.pack_into(frt_seq);
            parlay::parallel_for(0, frontier_size, [&](size_t i) {
                uint32_t s = frt_seq[i];
                result[s] = round;
                ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                    uint32_t d = G.edges[i].v;
                    if (active.try_deactivate(d)) { 
                        frontier.insert(d);
                    }
                });
            });
        }
        else {  // Direction Optimization
            active.parallel_do([&](size_t s) {
                for (size_t i = G.offsets[s]; i < G.offsets[s+1]; i++) { 
                    uint32_t d = G.edges[i].v;
                    if (!active.is_active(d)) { 
                        frontier.insert(s); result[s] = round;
                        return;
                    }
                }
            });
            if (!frontier.advance()) break;
            frontier.parallel_do([&](uint32_t s) { active.deactivate(s); });
        }
    }
    return result;
}