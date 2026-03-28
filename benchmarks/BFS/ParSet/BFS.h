#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s=0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = ParSet::Active(n); 
    auto frontier = ParSet::Frontier(n);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    frontier.insert_next(s);
    active.remove(s);
    for (uint32_t round = 0; ; round++) {
        if (mode == 0 || mode == 2) {
            if (!frontier.advance_to_next()) break;
            if (mode == 0 && round < uint32_t(2 * std::log(n)) && frontier.reduce_edge(G) > (G.m >> 3)) {
                frontier.for_each([&](uint32_t s) { result[s] = round; });
                mode = 1; continue;
            }
            frontier.for_each([&](uint32_t s) { 
                result[s] = round;
                parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                    int32_t d = G.edges[i].v;
                    if (active.contains(d)) {
                        active.remove(d); frontier.insert_next(d);
                    }
                }, 256);
            });
        }
        else {  // Direction Optimization
            active.for_each([&](size_t s) {
                for (size_t i = G.in_offsets[s]; i < G.in_offsets[s+1]; i++) { 
                    uint32_t d = G.in_edges[i].v;
                    if (!active.contains(d)) { 
                        frontier.insert_next(s); 
                        return;
                    }
                }
            });
            if (!frontier.advance_to_next()) break;
            if (frontier.reduce_vertex() < (G.n >> 6)) {
                frontier.for_each([&](uint32_t s) { frontier.insert_next(s); active.remove(s); });
                round--; mode = 2; continue;
            }
            frontier.for_each([&](uint32_t s) { result[s] = round; active.remove(s); });
        }
    }
    return result;
}