#pragma once
#include "vertex_set.h"

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s=0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = Active(n);
    auto frontier = Frontier(n);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    frontier.insert(s);
    active.mark_dead(s);

    for (uint32_t round = 0; frontier.round_switch(); round++) {
        if (mode == 0) { 
            if (frontier.reduce_edge(G) > 0.1 * G.m) {
                frontier.parallel_do([&](uint32_t s) { result[s] = round; });
                round++;
                active.parallel_do([&](size_t s) {
                    for (size_t j = G.offsets[s]; j < G.offsets[s+1]; j++) { uint32_t d = G.edges[j].v;
                        if (!active.is_active(d)) { 
                            frontier.insert(s); result[s] = round;
                            return;
                        }
                    }
                });
                mode = 1;
            }
            else {
                frontier.EdgeMapSparse(G, 
                    [&](uint32_t s) { result[s] = round; },
                    [&](uint32_t d) { return active.is_active(d); },
                    [&](uint32_t d) { active.mark_dead(d); frontier.insert(d); }
                );
                if (round == uint32_t(2 * std::log(n))) { mode = 2; frontier.set_fork_depth(4); }
            }
        }
        else if (mode == 1) { 
            frontier.parallel_do([&](uint32_t s) {
                if (active.is_active(s)) active.mark_dead(s); 
            });
            active.parallel_do([&](size_t s) {
                for (size_t j = G.offsets[s]; j < G.offsets[s+1]; j++) { uint32_t d = G.edges[j].v;
                    if (!active.is_active(d)) { 
                        frontier.insert(s); result[s] = round;
                        return;
                    }
                }
            });
        }
        else {
            frontier.EdgeMapSparse(G, 
                [&](uint32_t s) { result[s] = round; },
                [&](uint32_t d) { return active.is_active(d); },
                [&](uint32_t d) { active.mark_dead(d); frontier.insert(d); }
            );
        }
    }
    return result;
}
