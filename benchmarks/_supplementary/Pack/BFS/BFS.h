#pragma once
#include <ParSet/ParSet.h>
#include <cmath>

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s = 0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = ParSet::Active(n);
    auto frontier = ParSet::Frontier(n);
    auto frontier_vertices = parlay::sequence<uint32_t>(n, 0);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX);

    frontier.insert_next(s);
    active.remove(s);
    for (uint32_t round = 0;; round++) {
        if (mode == 0 || mode == 2) {
            if (!frontier.advance_to_next()) break;
            if (mode == 0 && round < uint32_t(2 * std::log(n)) && frontier.reduce_edge(G) > (G.m >> 3)) {
                size_t frontier_size = frontier.pack_into(frontier_vertices);
                parlay::parallel_for(0, frontier_size, [&](size_t i) {
                    result[frontier_vertices[i]] = round;
                });
                mode = 1; continue;
            }
            size_t frontier_size = frontier.pack_into(frontier_vertices);
            parlay::parallel_for(0, frontier_size, [&](size_t i) {
                uint32_t u = frontier_vertices[i];
                result[u] = round;
                ParSet::adaptive_for(G.offsets[u], G.offsets[u + 1], [&](size_t j) {
                    uint32_t v = G.edges[j].v;
                    if (active.try_remove(v)) {
                        frontier.insert_next(v);
                    }
                });
            });
        } 
        else {
            active.for_each([&](size_t u) {
                for (size_t i = G.offsets[u]; i < G.offsets[u + 1]; i++) {
                    uint32_t v = G.edges[i].v;
                    if (!active.contains(v)) {
                        frontier.insert_next(u);
                        return;
                    }
                }
            });
            if (!frontier.advance_to_next()) break;
            size_t frontier_size = frontier.pack_into(frontier_vertices);
            if (frontier_size < (G.n >> 6)) {
                parlay::parallel_for(0, frontier_size, [&](size_t i) {
                    uint32_t u = frontier_vertices[i];
                    frontier.insert_next(u);
                    active.remove(u);
                });
                round--;
                mode = 2;
                continue;
            }
            parlay::parallel_for(0, frontier_size, [&](size_t i) {
                uint32_t u = frontier_vertices[i];
                result[u] = round;
                active.remove(u);
            });
        }
    }
    return result;
}
