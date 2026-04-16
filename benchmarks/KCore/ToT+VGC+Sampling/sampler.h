#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <limits>

static constexpr uint32_t sample_threshold = 20000;
static constexpr double init_reduce_ratio = 0.1;
static constexpr uint32_t log2_error_factor = 32;
static constexpr double bias_factor = 0.5;
static constexpr double error_rate_tolerance = 0.0000000001;
static constexpr uint32_t exp_hits = log2_error_factor / (init_reduce_ratio * init_reduce_ratio);
static constexpr uint32_t min_threshold_sample_degree = static_cast<uint32_t>(sample_threshold / init_reduce_ratio);
static constexpr uint32_t min_exp_sample_degree = static_cast<uint32_t>(exp_hits / (1 - init_reduce_ratio)) + 1;
static constexpr uint32_t min_initial_sample_degree = std::max(min_threshold_sample_degree, min_exp_sample_degree);
static constexpr double log_error_rate_tolerance = std::log(error_rate_tolerance);

struct Sampler {
    uint32_t num_hits;
    uint32_t exp_hits;
    uint32_t threshold;

    Sampler(const size_t _exp_hits, double _sample_rate): exp_hits(_exp_hits), threshold(_sample_rate * UINT32_MAX), num_hits(0){}
    
    bool sample(uint32_t random_number) {
        if (num_hits >= exp_hits || random_number >= threshold) { return false; }
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
