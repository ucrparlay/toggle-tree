#pragma once
#include <ParSet/ParSet.h>

static inline bool write_min(int32_t& ref, int32_t v){
    int32_t old = ref;
    while (old > v && !__atomic_compare_exchange_n(&ref, &old, v, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old > v;
}

template <class Graph>
parlay::sequence<int32_t> Dijkstra(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto active = ParSet::Active(n, false); 
    auto frontier = ParSet::Frontier(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    active.insert(source); 
    parlay::internal::timer t; double t1=0,t2=0,t3=0;
    for (uint32_t round = 0; ;round++) {
        if(active.empty()) { std::cerr << "round=" << round << "\n"; break; }
        t.start();
        int32_t k = active.reduce_min(dist);
        t1 += t.stop();
        t.start();
        active.for_each([&](uint32_t s) { 
            if (dist[s] == k) { active.remove(s); frontier.insert_next(s);}
        });
        t2 += t.stop();
        t.start();
        if(!frontier.advance_to_next()) { std::cerr << "round=" << round << "\n"; break; }
        frontier.for_each([&](uint32_t s) { 
            int32_t dist_s = dist[s];
            ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].v;
                int32_t w = G.edges[i].w;
                if (dist[d] > dist_s + w && write_min(dist[d], dist_s + w)) {
                    active.insert(d);
                }
            });
        });
        t3 += t.stop();
    }
    std::cerr << "\nt1 = " << t1 << "\n";
    std::cerr << "t2 = " << t2 << "\n";
    std::cerr << "t3 = " << t3 << "\n";
    return dist;
}