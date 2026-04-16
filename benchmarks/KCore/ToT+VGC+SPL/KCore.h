#pragma once

#include <toggle/toggle.h>

static constexpr uint32_t sample_threshold = 20000;
static constexpr double init_reduce_ratio = 0.1;
static constexpr uint32_t log2_error_factor = 32;
static constexpr double bias_factor = 0.5;
static constexpr double error_rate_tolerance = 0.0000000001;
static constexpr double log_error_rate_tolerance = std::log(error_rate_tolerance);
static constexpr uint32_t min_initial_sample_degree = std::max(
    static_cast<uint32_t>(log2_error_factor / (init_reduce_ratio * init_reduce_ratio) / (1 - init_reduce_ratio)) + 1, 
    static_cast<uint32_t>(sample_threshold / init_reduce_ratio)
);

struct Sampler {
    uint32_t num_hits;
    uint32_t exp_hits;
    uint32_t threshold;

    Sampler(const size_t _exp_hits, double _sample_rate): exp_hits(_exp_hits), threshold(_sample_rate * UINT32_MAX), num_hits(0){}

    bool sample() {
        static thread_local uint32_t x=0x9e3779b9u;
        x = x*4294967291u + 374761393u;
        if (num_hits >= exp_hits || x >= threshold) { return false; }
        return __atomic_fetch_add(&num_hits, 1, __ATOMIC_RELAXED) + 1 == exp_hits;
    }

    void reset(uint32_t k, uint32_t deg) {
        int n_star = static_cast<int>(deg - k);
        uint32_t log_n_star = parlay::log2_up(n_star);
        size_t modified_exp_hits = log2_error_factor * log_n_star * log_n_star / 2;
        double sample_rate = static_cast<double>(modified_exp_hits) / ((1 - init_reduce_ratio) * deg);
        exp_hits = modified_exp_hits;
        threshold = sample_rate * UINT32_MAX;
        num_hits = 0;
    }

    bool unsafe(uint32_t k, uint32_t deg) {
        if (deg * init_reduce_ratio * bias_factor < k) { return true; }
        double n_star = static_cast<double>(deg - k);
        size_t num_hits = std::max<uint32_t>(1, num_hits);
        double sample_rate = static_cast<double>(exp_hits) / ((1 - init_reduce_ratio) * deg);
        double mu = n_star * sample_rate;
        double h = static_cast<double>(num_hits);
        return -mu + 2 * h - h * h / mu >= log_error_rate_tolerance;
    }

    bool require_sampling(uint32_t k, uint32_t deg) {
        return deg * init_reduce_ratio >= sample_threshold && k < deg * init_reduce_ratio * bias_factor && exp_hits < (deg - k) * (1 - init_reduce_ratio);
    }
};


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

        sample_mode.for_each([&](uint32_t s) {
            if (samplers[s].unsafe(k, coreness[s])) {
                coreness[s] = parlay::count_if(G.edges.cut(G.offsets[s], G.offsets[s + 1]), [&](auto& es) { 
                    return active.contains(es.idx) || frontier.contains_next(es.idx); 
                });
                if (coreness[s] < k) { coreness[s] = k; active.remove(s); sample_mode.remove(s); frontier.insert_next(s); }
                if (samplers[s].require_sampling(k, coreness[s])) { samplers[s].reset(k, coreness[s]); } 
                else { sample_mode.remove(s);}
            }
        });

        active.for_each([&](uint32_t s) {
            if (coreness[s] == k) {
                if (sample_mode.contains(s)) { sample_mode.remove(s); }
                frontier.insert_next(s);
                active.remove(s);
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
                            if (f_cond_w(d)) {
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
                                if (f_cond_w(d)) {
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
                    [&] (uint32_t d) { return __atomic_fetch_sub(&coreness[d], 1, __ATOMIC_RELAXED) == k + 1; },
                    [&] (uint32_t d) { active.remove(d); }
                ); });
            }
            else {
                frontier.for_each([&](uint32_t s) { local_search(local_search, s, 0, 
                    [&] (uint32_t s) { result[s] = k; },
                    [&] (uint32_t d) { return active.contains(d); },
                    [&] (uint32_t d) { 
                        if (sample_mode.contains(d)) {
                            if (samplers[d].sample()) { counting_set.insert(d); }
                            return false;
                        }
                        return __atomic_fetch_sub(&coreness[d], 1, __ATOMIC_RELAXED) == k + 1; 
                    },
                    [&] (uint32_t d) { active.remove(d); }
                ); });
            }
            counting_set.for_each<true>([&](uint32_t s) {
                coreness[s] = parlay::count_if(G.edges.cut(G.offsets[s], G.offsets[s + 1]), [&](auto& es) { 
                    return active.contains(es.idx) || frontier.contains_next(es.idx); 
                });
                if (coreness[s] < k) { coreness[s] = k; active.remove(s); sample_mode.remove(s); frontier.insert_next(s); }
                if (samplers[s].require_sampling(k, coreness[s])) { samplers[s].reset(k, coreness[s]); } 
                else { sample_mode.remove(s);}
            });
        }
    }
    return result;
}


