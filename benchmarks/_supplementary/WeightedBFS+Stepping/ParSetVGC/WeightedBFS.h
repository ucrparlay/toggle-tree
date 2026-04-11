#pragma once
#include <toggle/toggle.h>

template <class Graph, class Active>
void edgemap(Graph&G, uint32_t s, parlay::sequence<int32_t>& dist, Active& active, uint32_t maxiter) {
    int32_t dist_s = dist[s];
    parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
        uint32_t d = G.edges[i].idx;
        int32_t w = G.edges[i].w;
        if (dist[d] > dist_s + w && toggle::write_min(dist[d], dist_s + w)) {
            if (dist_s + w <= maxiter) {
                edgemap(G, d, dist, active, maxiter);
            }
            else { active.insert(d); }
        }
    }, 256);
}

template <class Graph>
parlay::sequence<int32_t> WeightedBFS(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto active = toggle::Active(n, false); 
    auto frontier = toggle::Frontier(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    active.insert(source); 
    uint32_t rho = 2000000;
    uint32_t mode = 0; double delta = 1; uint32_t lastex=0, thisex = 0;
    for (double round = 0; !active.empty(); ) {
        if (mode != uint32_t(std::log(n))) {
            thisex = active.reduce<uint32_t,0>(
                [&](uint32_t s) { 
                    if (dist[s] <= round) { active.remove(s); frontier.insert_next(s); return 1; } 
                    else return 0;
                },
                [&](uint32_t a, uint32_t b) { return a + b; }
            );
            round++;
            if (lastex > std::log(n) && thisex > std::log(n)) {
                if (mode % 2 == 0 && lastex > thisex) { mode++; thisex = 0; }
                else if (mode % 2 == 1 && lastex < thisex) { mode++; thisex = 0; }
            }
            lastex = thisex;
            frontier.advance_to_next();
            frontier.for_each([&](uint32_t s) { 
                int32_t dist_s = dist[s];
                parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                    uint32_t d = G.edges[i].idx;
                    int32_t w = G.edges[i].w;
                    if (dist[d] > dist_s + w && toggle::write_min(dist[d], dist_s + w)) {
                        active.insert(d);
                    }
                }, 256);
            });
        }
        else {
            uint64_t extracted = 0; uint32_t cnt = 0;
            while (!active.empty() && extracted < rho) {
                cnt++;
                round += delta;
                extracted += active.reduce<uint32_t,0>(
                    [&](uint32_t s) { 
                        if (dist[s] <= round) { active.remove(s); frontier.insert_next(s); return 1; } 
                        else return 0;
                    },
                    [&](uint32_t a, uint32_t b) { return a + b; }
                );
            }
            if (extracted >= rho) delta = (delta * cnt * rho) / extracted;
            else delta = delta * cnt;
            frontier.advance_to_next();
            frontier.for_each([&](uint32_t s) { 
                edgemap(G, s, dist, active, round);
            });
        }
    }
    return dist;
}