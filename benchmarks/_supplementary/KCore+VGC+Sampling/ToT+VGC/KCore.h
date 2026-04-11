#pragma once
#include <toggle/toggle.h>

#include <array>

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);
    auto active = toggle::Active(n);
    auto frontier = toggle::Frontier(n);
    auto D = parlay::tabulate<uint32_t>(n, [&](size_t s) {
        if (G.offsets[s + 1] - G.offsets[s] == 0) {
        active.remove(s);
        }
        return G.offsets[s + 1] - G.offsets[s];
    });

    parlay::internal::timer t;
    double t0 = 0;
    while (!active.empty()) {
        uint32_t k = active.reduce_min(D);
        active.for_each([&](uint32_t s) {
            if (D[s] == k) {
                active.remove(s);
                frontier.insert_next(s);
            }
        });
        while (frontier.advance_to_next()) {
            const size_t LOCAL_QUEUE = 128;
            const size_t STRIDE = 8;
            auto local_search = [&](auto&& self, uint32_t s, uint32_t layer) -> void {
                if (G.offsets[s+1]-G.offsets[s] >= LOCAL_QUEUE) {
                    result[s] = k;
                    parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                        uint32_t d = G.edges[i].idx;
                        if (active.contains(d)) {
                            if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k + 1) {
                                active.remove(d);
                                if (layer+1==STRIDE)frontier.insert_next(d);
                                else self(self, d, layer+1);
                            }
                        }
                    }, LOCAL_QUEUE);
                }
                else {
                    uint32_t local_queue[LOCAL_QUEUE];
                    local_queue[0] = s;
                    size_t lpos = 0, rpos = 1, cnt = 1;
                    while (lpos < rpos && cnt < LOCAL_QUEUE) {
                        uint32_t cur_index = local_queue[lpos];
                        result[cur_index] = k;
                        if (G.offsets[cur_index+1]-G.offsets[cur_index]+rpos>LOCAL_QUEUE) { break; }
                        lpos++;
                        for (size_t i=G.offsets[cur_index]; i<G.offsets[cur_index+1]; i++) {
                            cnt++; uint32_t d = G.edges[i].idx;
                            if (active.contains(d)) {
                                if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) == k + 1) {
                                    active.remove(d);
                                    local_queue[rpos] = d; rpos++;
                                }
                            }
                        }
                    }
                    parlay::parallel_for(lpos,rpos,[&](uint32_t i){ 
                        if (layer+1 == STRIDE) frontier.insert_next(local_queue[i]); 
                        else self(self, local_queue[i], layer+1);
                    },1);
                }
            };
            frontier.for_each([&](uint32_t s) { local_search(local_search, s, 0); });
        }
    }

    return result;
}
