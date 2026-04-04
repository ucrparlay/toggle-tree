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
            if (round == uint32_t(2 * std::log(n))) {
                frontier.for_each([&](uint32_t s) { 
                    result[s] = round;
                    parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                        uint32_t d = G.edges[i].v;
                        if (active.contains(d)) {
                            active.remove(d); frontier.insert_next(d); result[d] = round+1;
                        }
                    }, 256);
                });
                round++; mode = 3; continue;
            }
            frontier.for_each([&](uint32_t s) { 
                result[s] = round;
                parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                    uint32_t d = G.edges[i].v;
                    if (active.contains(d)) {
                        active.remove(d); frontier.insert_next(d);
                    }
                }, 256);
            });
        }
        else if (mode == 1) {  // Direction Optimization
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
        else {  // Local Search
            if (!frontier.advance_to_next()) break;
            static const size_t gra = 4;
            frontier.for_each([&](size_t s) {
                parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](uint32_t j) { 
                    uint32_t d = G.edges[j].v;
                    if (ParSet::write_min(result[d], round)) { 
                        parlay::parallel_for(G.offsets[d], G.offsets[d+1], [&](uint32_t k) { 
                            uint32_t u = G.edges[k].v;
                            if (ParSet::write_min(result[u], round + 1)) { 
                                parlay::parallel_for(G.offsets[u], G.offsets[u+1], [&](uint32_t l) { 
                                    uint32_t w = G.edges[l].v;
                                    if (ParSet::write_min(result[w], round + 2)) { 
                                        parlay::parallel_for(G.offsets[w], G.offsets[w+1], [&](uint32_t m) { 
                                            uint32_t x = G.edges[m].v;
                                            if (ParSet::write_min(result[x], round + 3)) { 
                                                parlay::parallel_for(G.offsets[x], G.offsets[x+1], [&](uint32_t o) { 
                                                    uint32_t y = G.edges[o].v;
                                                    if (ParSet::write_min(result[y], round + 4)) { 
                                                        frontier.insert_next(y); 
                                                    }
                                                }, gra);
                                            }
                                        }, gra);
                                    }
                                }, gra);
                            }
                        }, gra);
                    }
                }, gra);
            });
            round+=4;
        }
    }
    return result;
}
