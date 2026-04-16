#pragma once

#include <toggle/toggle.h>
#include "sampler.h"

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
    const size_t n = G.n;
    auto result = parlay::sequence<uint32_t>(n, 0);
    auto coreness = parlay::sequence<uint32_t>::uninitialized(n);
    auto active = toggle::Active(n);
    auto frontier = toggle::Frontier(n);
    
    auto counting_set = toggle::Active(n, false);
    auto sample_mode = toggle::Active(n, false);
    auto samplers = parlay::sequence<Sampler>::uninitialized(n);

    parlay::parallel_for(0, n, [&](size_t i) {
        coreness[i] = G.offsets[i + 1] - G.offsets[i];
        if (coreness[i] == 0) { active.remove(i); }
        if (coreness[i] >= min_initial_sample_degree) { 
            sample_mode.insert(i);
            samplers[i].reset(0, coreness[i]);
        }
    });

    uint32_t k = 0; int32_t use_reduce = -2*std::log2(n);
    while (!active.empty()) {
        k = use_reduce>=0 ? active.reduce_min(coreness) : k+1;

        sample_mode.for_each([&](uint32_t u) {
            if (samplers[u].unsafe(k, coreness[u])) {
                coreness[u] = parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]), [&](auto& es) { 
                    return active.contains(es.idx) || frontier.contains_next(es.idx); 
                });
                if (coreness[u] < k) { coreness[u] = k; active.remove(u); sample_mode.remove(u); frontier.insert_next(u); }
                if (samplers[u].require_sampling(k, coreness[u])) { samplers[u].reset(k, coreness[u]); } 
                else { sample_mode.remove(u);}
            }
        });

        active.for_each([&](uint32_t u) {
            if (coreness[u] == k) {
                if (sample_mode.contains(u)) { sample_mode.remove(u); }
                frontier.insert_next(u);
                active.remove(u);
            }
        });

        if (use_reduce<0 && frontier.empty_next()) { use_reduce++; }

        while (frontier.advance_to_next()) {
            const size_t LOCAL_QUEUE = 128;
            const size_t STRIDE = 8;
            auto local_search = [&](auto&& self, uint32_t s, uint32_t layer, auto f_source, auto f_cond_r, auto f_cond_w, auto f_dest) -> void {
                if (G.offsets[s+1]-G.offsets[s] >= LOCAL_QUEUE) {
                    result[s] = k;
                    f_source(s);
                    parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                        uint32_t d = G.edges[i].idx;
                        if (f_cond_r(d)) {
                            if (f_cond_w(s, d)) {
                                f_dest(d);
                                if (layer+1==STRIDE) frontier.insert_next(d);
                                else self(self, d, layer+1, f_source, f_cond_r, f_cond_w, f_dest);
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
                            if (f_cond_r(d)) {
                                if (f_cond_w(cur_index, d)) {
                                    f_dest(d);
                                    local_queue[rpos] = d; rpos++;
                                }
                            }
                        }
                    }
                    parlay::parallel_for(lpos,rpos,[&](uint32_t i){ 
                        if (layer+1 == STRIDE) frontier.insert_next(local_queue[i]); 
                        else self(self, local_queue[i], layer+1, f_source, f_cond_r, f_cond_w, f_dest);
                    },1);
                }
            };
            if (sample_mode.empty()) {
                frontier.for_each([&](uint32_t s) { local_search(local_search, s, 0, 
                    [&] (uint32_t s) { result[s] = k; },
                    [&] (uint32_t d) { return active.contains(d); },
                    [&] (uint32_t s, uint32_t d) { return __atomic_fetch_sub(&coreness[d], 1, __ATOMIC_RELAXED) == k + 1; },
                    [&] (uint32_t d) { active.remove(d); }
                ); });
            }
            else {
                /*
                frontier.for_each([&](uint32_t u) {
                    result[u] = k;
                    parlay::parallel_for(G.offsets[u], G.offsets[u + 1], [&](size_t i) {
                        uint32_t v = G.edges[i].idx;
                        if (!active.contains(v)) { return; }
                        if (sample_mode.contains(v)) { 
                            if (samplers[v].sample(parlay::hash32(u * G.n + v))) {
                                counting_set.insert(v);
                            }
                        } 
                        else {
                            if (__atomic_fetch_sub(&coreness[v], 1, __ATOMIC_RELAXED) == k + 1) {
                                active.remove(v);
                                frontier.insert_next(v);
                            }
                        }
                    }, 256);
                });*/
                frontier.for_each([&](uint32_t s) { local_search(local_search, s, 0, 
                    [&] (uint32_t s) { result[s] = k; },
                    [&] (uint32_t d) { return active.contains(d); },
                    [&] (uint32_t s, uint32_t d) { 
                        if (sample_mode.contains(d)) {
                            if (samplers[d].sample(parlay::hash32(s * G.n + d))) { counting_set.insert(d); }
                            return false;
                        }
                        return __atomic_fetch_sub(&coreness[d], 1, __ATOMIC_RELAXED) == k + 1; 
                    },
                    [&] (uint32_t d) { active.remove(d); }
                ); });
            }
            counting_set.for_each<true>([&](uint32_t u) {
                coreness[u] = parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]), [&](auto& es) { 
                    return active.contains(es.idx) || frontier.contains_next(es.idx); 
                });
                if (coreness[u] < k) { coreness[u] = k; active.remove(u); sample_mode.remove(u); frontier.insert_next(u); }
                if (samplers[u].require_sampling(k, coreness[u])) { samplers[u].reset(k, coreness[u]); } 
                else { sample_mode.remove(u);}
            });
        }
    }
    return result;
}
