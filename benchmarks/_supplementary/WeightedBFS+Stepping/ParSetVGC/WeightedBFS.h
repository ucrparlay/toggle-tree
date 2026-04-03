#pragma once
#include <ParSet/ParSet.h>

template <class Graph, class Active>
void edgemap(Graph&G, uint32_t s, parlay::sequence<int32_t>& dist, Active& active, uint32_t maxiter) {
    int32_t dist_s = dist[s];
    parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
        uint32_t d = G.edges[i].v;
        int32_t w = G.edges[i].w;
        if (dist[d] > dist_s + w && ParSet::write_min(dist[d], dist_s + w)) {
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
    auto active = ParSet::Active(n, false); 
    auto frontier = ParSet::Frontier(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    active.insert(source); 
    uint32_t rho = 400000;
    double delta = 1;
    parlay::internal::timer t; double t1=0, t2=0;
    bool enable = false; uint64_t lastex, thisex = 0;
    for (double round = 0; !active.empty(); ) {
        t.start();
        uint64_t extracted = 0; uint64_t cnt = 0;
        while (!active.empty() && extracted < rho) {
            //std::cerr << "  " << extracted << "\n";
            cnt++;
            round+=delta;
            thisex = active.reduce<uint64_t,0>(
                [&](uint32_t s) { 
                    if (dist[s] <= round) { active.remove(s); frontier.insert_next(s); return G.offsets[s + 1] - G.offsets[s]; } 
                    else return uint64_t(0);
                },
                [&](uint64_t a, uint64_t b) { return a + b; }
            );
            if (lastex > thisex) enable = 1;
            lastex = thisex;
            if (enable == false) break;
            extracted += thisex;
        }
        std::cerr << "extracte = " << extracted << "\n";
        if (enable) {
            if (extracted >= rho) {
            //    std::cerr << delta << "  " << cnt << "  " << extracted << "  " << rho << "\n";
                delta = (delta * cnt * rho) / extracted;
            }
            else {
                delta = delta * cnt;
            }
        }
        
        //std::cerr << cnt << "\n";
        t1 += t.stop();
        t.start();
        frontier.advance_to_next();
        frontier.for_each([&](uint32_t s) { 
            edgemap(G, s, dist, active, round);
        });
        t2 += t.stop();
    }
    //std::cerr << "\nt1=" << t1 << "  t2=" << t2 << "\n";
    
    return dist;
}