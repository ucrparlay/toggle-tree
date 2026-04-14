#pragma once
#include <toggle/toggle.h>

struct Sampler {
    uint32_t num_hits;
    uint32_t exp_hits;
    uint32_t threshold;

    Sampler(const size_t _exp_hits, double _sample_rate): exp_hits(_exp_hits), threshold(_sample_rate * UINT32_MAX), num_hits(0) { }

    bool sample(uint32_t random_number) {
        if (num_hits >= exp_hits) { return false; }
        if (random_number < threshold) {
            uint32_t ret = __atomic_fetch_add(&num_hits, 1, __ATOMIC_RELAXED);
            if (ret >= exp_hits) { return false; } 
            else if (ret + 1 == exp_hits) { return true; }
        }
        return false;
    }

    void reset(uint32_t v, uint32_t k, uint32_t &D, toggle::Active &sample_mode) {
        constexpr uint32_t sample_threshold = 2000;
        constexpr double init_reduce_ratio = 0.1;
        constexpr uint32_t log2_error_factor = 32;
        constexpr double bias_factor = 0.5;
        constexpr uint32_t exp_hits_0 = log2_error_factor / (init_reduce_ratio * init_reduce_ratio);
        if (D * init_reduce_ratio >= sample_threshold && k < D * init_reduce_ratio * bias_factor && exp_hits_0 < (D - k) * (1 - init_reduce_ratio)) {
            sample_mode.insert(v);
            exp_hits = log2_error_factor * parlay::log2_up((int)(D - k)) * parlay::log2_up((int)(D - k)) / 2;
            threshold = exp_hits / ((1 - init_reduce_ratio) * D) * UINT32_MAX;
            num_hits = 0;
        } else { sample_mode.remove(v); }
    }
};

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph &G) {
    constexpr uint32_t sample_threshold = 2000;
    constexpr double init_reduce_ratio = 0.1;
    constexpr double bias_factor = 0.5;
    constexpr double error_rate_tolerance = 0.0000000001;
    constexpr uint32_t sampling_window = 64;
    constexpr size_t LOCAL_QUEUE = 128;
    constexpr size_t STRIDE = 8;

    auto D = parlay::sequence<uint32_t>::uninitialized(G.n);
    auto samplers = parlay::sequence<Sampler>::uninitialized(G.n);
    auto result = parlay::sequence<uint32_t>(G.n, 0);
    toggle::Frontier frontier(G.n);
    toggle::Active active(G.n);
    toggle::Active sample_mode(G.n, false);
    toggle::Active frontier_resample(G.n, false);

    parlay::parallel_for(0, G.n, [&](size_t u) {
        D[u] = G.offsets[u + 1] - G.offsets[u];
        if (D[u] * init_reduce_ratio >= sample_threshold) {
            samplers[u].reset(u, 0, D[u], sample_mode);
        }
    });

    uint32_t k = 0; int32_t use_reduce = -2*std::log2(G.n);
    while (!active.empty()) {
        k = use_reduce>=0 ? active.reduce_min(D) : k+1;
        active.for_each([&](uint32_t u) {
            if (D[u] == k) { frontier.insert_next(u); }
            if (sample_mode.contains(u)) {
                double error_rate = 1;
                double sample_rate = samplers[u].exp_hits / ((1 - init_reduce_ratio) * D[u]);
                if (D[u] * init_reduce_ratio * bias_factor >= (k + sampling_window)) {
                    int n_star = D[u] - (k + sampling_window);
                    size_t num_hits = std::max((uint32_t)1, samplers[u].num_hits);
                    error_rate = std::exp(-1.0 * n_star * sample_rate + 2 * num_hits - 1.0 * num_hits * num_hits / n_star / sample_rate);
                }
                if (error_rate >= error_rate_tolerance) {
                    D[u] = parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]), [&](auto &es) { return active.contains(es.idx); });
                    if (D[u]< k) D[u] = k;
                    samplers[u].reset(u, k, D[u], sample_mode);
                }
            }
        });
        
        if (use_reduce<0 && frontier.empty_next()) { use_reduce++; }

        while (frontier.advance_to_next()) {
            auto local_search = [&](auto&& self, uint32_t s, uint32_t layer) -> void {
                if (G.offsets[s + 1] - G.offsets[s] >= LOCAL_QUEUE) {
                    active.remove(s);
                    result[s] = k;
                    parlay::parallel_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                        uint32_t v = G.edges[i].idx;
                        if (active.contains(v)) {
                            if (sample_mode.contains(v)) {
                                if (samplers[v].sample(parlay::hash32(s * G.n + v))) { frontier_resample.insert(v); }
                            } else if (__atomic_fetch_sub(&D[v], 1, __ATOMIC_RELAXED) == k + 1) {
                                if (layer + 1 == STRIDE) frontier.insert_next(v);
                                else self(self, v, layer + 1);
                            }
                        }
                    }, LOCAL_QUEUE);
                } else {
                    uint32_t local_queue[LOCAL_QUEUE];
                    local_queue[0] = s;
                    size_t lpos = 0, rpos = 1, cnt = 1;
                    while (lpos < rpos && cnt < LOCAL_QUEUE) {
                        uint32_t u = local_queue[lpos];
                        active.remove(u);
                        result[u] = k;
                        if (G.offsets[u + 1] - G.offsets[u] + rpos > LOCAL_QUEUE) { break; }
                        lpos++;
                        for (size_t i = G.offsets[u]; i < G.offsets[u + 1]; i++) {
                            cnt++;
                            uint32_t v = G.edges[i].idx;
                            if (active.contains(v)) {
                                if (sample_mode.contains(v)) {
                                    if (samplers[v].sample(parlay::hash32(u * G.n + v))) { frontier_resample.insert(v); }
                                } else if (__atomic_fetch_sub(&D[v], 1, __ATOMIC_RELAXED) == k + 1) {
                                    local_queue[rpos] = v;
                                    rpos++;
                                }
                            }
                        }
                    }
                    parlay::parallel_for(lpos, rpos, [&](uint32_t i) { 
                        if (layer + 1 == STRIDE) frontier.insert_next(local_queue[i]); 
                        else self(self, local_queue[i], layer + 1);
                    }, 1);
                }
            };
            frontier.for_each([&](uint32_t u) { local_search(local_search, u, 0); });
            frontier_resample.for_each<true>([&](uint32_t u) {
                D[u] = parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]), [&](auto &es) { return active.contains(es.idx); });
                if (D[u] <= k) {
                    D[u] = k;
                    frontier.insert_next(u);
                } else { samplers[u].reset(u, k, D[u], sample_mode); }
            });
            
        }
    }

    return result;
}
