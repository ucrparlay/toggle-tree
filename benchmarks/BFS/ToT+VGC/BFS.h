#pragma once
#include <toggle/toggle.h>



template <class Graph>
parlay::sequence<uint32_t> BFS(Graph& G, size_t s=0) {
    const size_t n = G.n;
    uint8_t mode = 0;
    auto active = toggle::Active(n); 
    auto frontier = toggle::Frontier(n);
    auto result = parlay::sequence<uint32_t>(n, UINT32_MAX); 
    frontier.insert_next(s);
    active.remove(s);

    uint32_t last_size = 0, this_size = 0; int32_t use_vgc = -std::log2(n);;
    for (uint32_t round = 0; ; round++) {
        if (mode == 0) {
            if (!frontier.advance_to_next()) break;
            if (frontier.reduce_edge(G) > (G.m >> 3)) {
                frontier.for_each([&](uint32_t s) { result[s] = round; });
                mode = 1; continue;
            }
            frontier.for_each([&](uint32_t s) { 
                result[s] = round;
                parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                    uint32_t d = G.edges[i].idx;
                    if (active.contains(d)) {
                        active.remove(d); frontier.insert_next(d);
                    }
                }, 256);
            });
            if (round == uint32_t(std::log(n))) mode = 2;
        }
        else if (mode == 1) {  // Direction Optimization
            active.for_each([&](size_t s) {
                for (size_t i = G.in_offsets[s]; i < G.in_offsets[s+1]; i++) { 
                    uint32_t d = G.in_edges[i].idx;
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
        else if (mode == 2) {  
            if (!frontier.advance_to_next()) break;
            this_size = frontier.approximate_vertex();
            if (this_size < last_size) use_vgc++;
            last_size = this_size;
            if (use_vgc >= 0 && this_size < 100000) {
                frontier.for_each([&](uint32_t s) { 
                    result[s] = round;
                    parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                        uint32_t d = G.edges[i].idx;
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
                    uint32_t d = G.edges[i].idx;
                    if (active.contains(d)) {
                        active.remove(d); frontier.insert_next(d);
                    }
                }, 256);
            });
        }
        else {  // Local Search
            if (!frontier.advance_to_next()) break;
            const size_t LOCAL_QUEUE = 128;
            const size_t STRIDE = 8;
            
            auto local_search = [&](auto&& self, uint32_t s) -> void {
                if (G.offsets[s+1]-G.offsets[s] >= LOCAL_QUEUE) {
                    uint32_t dist = result[s];
                    parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                        uint32_t d = G.edges[i].idx;
                        if (toggle::write_min(result[d], dist+1)) {
                            if (dist+1==round+STRIDE) { frontier.insert_next(d); }
                            else { self(self, d); }
                        }
                    });
                }
                else {
                    uint32_t local_queue[LOCAL_QUEUE];
                    local_queue[0] = s;
                    size_t lpos = 0, rpos = 1, cnt = 1;
                    while (lpos < rpos && cnt < LOCAL_QUEUE) {
                        uint32_t cur_index = local_queue[lpos];
                        uint32_t cur_dist = result[cur_index];
                        if (G.offsets[cur_index+1]-G.offsets[cur_index]+rpos>LOCAL_QUEUE) { break; }
                        lpos++;
                        for (size_t i=G.offsets[cur_index]; i<G.offsets[cur_index+1]; i++) {
                            cnt++; uint32_t d = G.edges[i].idx;
                            if (toggle::write_min(result[d], cur_dist+1)) {
                                if (cur_dist+1==round+STRIDE) { frontier.insert_next(d); }
                                else { local_queue[rpos] = d; rpos++; }
                            }
                        }
                    }
                    parlay::parallel_for(lpos,rpos,[&](uint32_t i){ self(self, local_queue[i]); });
                }
            };
            frontier.for_each([&](uint32_t s) {
                local_search(local_search, s);
            });
            round=round+STRIDE-1;
        }
    }
    return result;
}
