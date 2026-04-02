#pragma once
#include <ParSet/ParSet.h>

double ant (ParSet::internal::ParallelBitmap& active) {
    static thread_local uint64_t rnd = 0x12345678u;
    uint64_t ret = 0;
    //parlay::parallel_for()
    for (uint32_t i=0;i<10000;i++) {
        rnd = rnd * 1664525u + 1013904223u;
        uint64_t idx = rnd % active.bitmap[5].size();
        ret += __builtin_popcountll(active.bitmap[5][idx]);
    }
    return double(ret) / double(640000);

};

template <class Graph>
parlay::sequence<int32_t> WeightedBFS(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto active = ParSet::Active(n, false); 
    auto frontier = ParSet::Frontier(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    active.insert(source); 
    parlay::internal::timer t; double tt = 0; uint32_t sze = 0; double ase=0;
    for (uint32_t round = 0; !active.empty() ;round+=2) {
        
        //t.start();
        //ase = ant(active.active);
        //ase = active.reduce_vertex();
        //tt += t.stop();
        
        std::cerr << active.reduce_vertex() << "  ";
        active.for_each([&](uint32_t s) { 
            if (dist[s] == round) { active.remove(s); frontier.insert_next(s);}
        });
        frontier.advance_to_next(); 
        std::cerr << frontier.reduce_vertex() << "\n";
        frontier.for_each([&](uint32_t s) { 
            int32_t dist_s = dist[s];
            parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].v;
                int32_t w = G.edges[i].w;
                if (dist[d] > dist_s + w && ParSet::write_min(dist[d], dist_s + w)) {
                    active.insert(d);
                }
            }, 256);
        });
        //if (ase*G.n == 5) std::cerr << (ase*G.n)<< " ";
        
    }
    std::cerr << "\n"<< "tt=" << tt << "\n";
    return dist;
}