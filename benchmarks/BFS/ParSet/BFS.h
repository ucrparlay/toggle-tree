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
    //parlay::internal::timer t;
    for (uint32_t round = 0; ; round++) {
        //std :: cerr << "round = " << round-1 << "  time = " << t.stop() << "\n";
        //t.start();
        if (mode == 0) {
            if (!frontier.advance_to_next()) break;
            if (round < uint32_t(2 * std::log(n)) && frontier.reduce_edge(G) > 0.15 * G.m) {
                frontier.for_each([&](uint32_t s) { result[s] = round; });
                mode = 1; continue;
            }
            //if (round == 3) std::cerr << "3: " << frontier.reduce_vertex() << "  " << frontier.reduce_edge(G) << "  " << G.n << "  " << G.m << "\n";
            //if (round == 4) std::cerr << "4: " << frontier.reduce_vertex() << "  " << frontier.reduce_edge(G) << "  " << G.n << "  " << G.m << "\n";
            frontier.for_each([&](uint32_t s) { 
                result[s] = round;
                ParSet::adaptive_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                    uint32_t d = G.edges[i].v;
                    if (active.try_remove(d)) { 
                        frontier.insert_next(d);
                    }
                });
            });
        }
        else {  // Direction Optimization
            active.for_each([&](size_t s) {
                for (size_t i = G.offsets[s]; i < G.offsets[s+1]; i++) { 
                    uint32_t d = G.edges[i].v;
                    if (!active.contains(d)) { 
                        frontier.insert_next(s); result[s] = round;
                        return;
                    }
                }
            });
            if (!frontier.advance_to_next()) break;
            frontier.for_each([&](uint32_t s) { active.remove(s); });
        }
    }
    return result;
}