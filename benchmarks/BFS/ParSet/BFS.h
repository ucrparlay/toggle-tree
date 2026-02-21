#pragma once
#include <ParSet/ParSet.h>

template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s=0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = ParSet::Active(n);
    auto frontier = ParSet::Frontier(n);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    frontier.insert(s);
    active.deactivate(s);
    for (uint32_t round = 0; ; round++) {
        if (mode == 0) { 
            if (!frontier.advance()) break;
            if (frontier.reduce_edge(G) > 0.1 * G.m) {
                frontier.parallel_do([&](uint32_t s) { result[s] = round; });
                mode = 1; frontier.coarse_granularity(); active.fine_granularity();
            }
            else {
                frontier.parallel_do([&](uint32_t s) { 
                    result[s] = round;
                    vertex_set::adaptive_for
                    ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                        uint32_t d = G.edges[i].v;
                        if (active.try_deactivate(d)) { 
                            frontier.insert(d);
                        }
                    });
                });
                if (round == uint32_t(2 * std::log(n))) { mode = 2; frontier.coarse_granularity(); }
            }
        }
        else if (mode == 1) { 
            active.parallel_do([&](size_t s) {
                for (size_t _ = G.offsets[s]; j < G.offsets[s+1]; j++) { 
                    uint32_t d = G.edges[j].v;
                    if (!active.is_active(d)) { 
                        frontier.insert(s); result[s] = round;
                        return;
                    }
                }
            });
            if (!frontier.advance()) break;
            frontier.parallel_do([&](uint32_t s) {
                if (active.is_active(s)) active.deactivate(s); 
            });
        }
        else {
            if (!frontier.advance()) break;
            frontier.parallel_do([&](uint32_t s) { 
                result[s] = round;
                ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](uint32_t i) { 
                    uint32_t d = G.edges[i].v;
                    if (active.try_deactivate(d)) { 
                        frontier.insert(d);
                    }
                });
            });
        }
    }
/*
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
            sparse_frontier.EdgeMapSparse(G, 
                [&](uint32_t s) { result[s] = round; },
                [&](uint32_t d) { return active.is_active(d); },
                [&](uint32_t d) { active.mark_dead(d); frontier.insert(d); }
            );
        }
    }
    return result;
    */
}
