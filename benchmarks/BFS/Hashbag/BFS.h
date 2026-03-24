#pragma once
#include <ParSet/ParSet.h>
#include <cmath>
#include "hashbag.h"

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s = 0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = ParSet::Active(n);
    hashbag<uint32_t> frontier(n);
    auto frontier_vertices = parlay::sequence<uint32_t>::uninitialized(n);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX);

    frontier.insert(s);
    active.remove(s);
    for (uint32_t round = 0;; round++) {
        if (mode == 0 || mode == 2) {
            size_t frontier_size = frontier.pack_into(parlay::make_slice(frontier_vertices));
            if (!frontier_size) break;
            if (mode == 0 && round < uint32_t(2 * std::log(n))) {
                size_t frontier_edges = parlay::reduce(
                    parlay::delayed_seq<size_t>(frontier_size, [&](size_t i) {
                        uint32_t u = frontier_vertices[i];
                        return G.offsets[u + 1] - G.offsets[u];
                    }
                ));
                if (frontier_edges > (G.m >> 3)) {
                    parlay::parallel_for(0, frontier_size, [&](size_t i) {
                        result[frontier_vertices[i]] = round;
                    });
                    mode = 1;
                    continue;
                }
            }
            parlay::parallel_for(0, frontier_size, [&](size_t i) {
                uint32_t u = frontier_vertices[i];
                result[u] = round;
                ParSet::adaptive_for(G.offsets[u], G.offsets[u + 1], [&](size_t j) {
                    uint32_t v = G.edges[j].v;
                    if (active.active.try_remove(v)) {
                        frontier.insert(v);
                    }
                });
            });
        } 
        else {
            active.for_each([&](size_t u) {
                for (size_t i = G.offsets[u]; i < G.offsets[u + 1]; i++) {
                    uint32_t v = G.edges[i].v;
                    if (!active.contains(v)) {
                        frontier.insert(u);
                        return;
                    }
                }
            });
            size_t frontier_size = frontier.pack_into(parlay::make_slice(frontier_vertices));
            if (!frontier_size) break;
            if (frontier_size < (G.n >> 6)) {
                parlay::parallel_for(0, frontier_size, [&](size_t i) {
                    uint32_t u = frontier_vertices[i];
                    frontier.insert(u);
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
