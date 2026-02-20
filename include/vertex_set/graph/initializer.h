#pragma once
#include "parlay/parallel.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"
#include "graph/graph.h"
#include "graph/edge_map_blocked.h"
#include "hashbag/hashbag.h"

template <class Counter, class F, class Bitmap>
void init_from_function(parlay::sequence<Counter>& counters, size_t n, F&& f, Bitmap& active, hashbag<uint32_t>& bag) {
    counters = parlay::sequence<Counter>::from_function(n, [&](size_t i) {
        uint32_t initial = std::forward<F>(f)(i);
        if (initial == 0) { active.mark_dead(i); bag.insert((uint32_t)i); }
        return Counter(initial);
    });
}

template <class Counter, class Graph, class Bitmap>
void init_from_degree(parlay::sequence<Counter>& counters, Graph& G, Bitmap& active, hashbag<uint32_t>& bag) {
    const size_t n = G.n;
    counters = parlay::sequence<Counter>::from_function(n, [&](size_t i) {
        uint32_t initial = G.offsets[i + 1] - G.offsets[i];
        if (initial == 0) { active.mark_dead(i); bag.insert((uint32_t)i); }
        return Counter(initial);
    });
}

template <class Counter, class Graph, class Perm, class Bitmap>
void init_from_permutation(parlay::sequence<Counter>& counters, Graph& G, Perm& priority, Bitmap& active, hashbag<uint32_t>& bag) {
    const size_t n = G.n;
    if (n < 2000'0000 || G.m < 1'0000'0000 || G.m < 3 * n) {
        counters = parlay::sequence<Counter>::from_function(n, [&](size_t i) {
            uint32_t our_perm = priority[i];
            size_t start = G.offsets[i];
            size_t end   = G.offsets[i + 1];
            size_t deg   = end - start;
            auto contrib = parlay::delayed_seq<size_t>(deg, [&](size_t j) {
                uint32_t d = G.edges[start + j].v;
                return (priority[d] < our_perm);
            });
            uint32_t initial = parlay::reduce(contrib);
            if (initial == 0) { active.mark_dead(i); bag.insert((uint32_t)i); }
            return Counter(initial);
        });
        return;
    }
    constexpr size_t SAMPLE_K = 100;
    uint32_t max_perm_est = 1;
    for (size_t i = 0; i < std::min(SAMPLE_K, n); i++) { max_perm_est = std::max(max_perm_est, priority[i]); }
    auto small_perm = parlay::sequence<uint8_t>(n);
    uint32_t bucket_size = (max_perm_est + 255) >> 8;
    parlay::parallel_for(0, n, [&](size_t i) {
        uint32_t b = priority[i] / bucket_size;
        if (b > 255) b = 255;
        small_perm[i] = static_cast<uint8_t>(b);
    });
    counters = parlay::sequence<Counter>::from_function(n, [&](size_t i) {
        uint8_t our_small = small_perm[i];
        size_t start = G.offsets[i];
        size_t end   = G.offsets[i + 1];
        size_t deg   = end - start;
        auto contrib = parlay::delayed_seq<size_t>(deg, [&](size_t j) {
            uint32_t d = G.edges[start + j].v;
            uint8_t sd = small_perm[d];
            if (sd != our_small) { return (sd < our_small); }
            return (priority[d] < priority[i]);
        });
        uint32_t initial = parlay::reduce(contrib);
        if (initial == 0) { active.mark_dead(i); bag.insert((uint32_t)i); }
        return Counter(initial);
    });
}

template <class Counter, class Graph, class Insert>
void init_from_priority(parlay::sequence<Counter>& counters, Graph& G, parlay::sequence<uint32_t>& priority1, parlay::sequence<uint32_t>& priority2, Insert&& insert) {
    const size_t n = G.n;
    if (n < 2000'0000 || G.m < 1'0000'0000 || G.m < 3 * n) {
        counters = parlay::sequence<Counter>::from_function(n, [&](size_t s) {
            size_t start = G.offsets[s];
            size_t end   = G.offsets[s + 1];
            size_t deg   = end - start;
            uint32_t priority1_s = priority1[s];
            uint32_t priority2_s = priority2[s];
            auto contrib = parlay::delayed_seq<size_t>(deg, [&](size_t j) {
                uint32_t d = G.edges[start + j].v;
                uint32_t priority1_d = priority1[d];
                return (priority1_d < priority1_s) || (priority1_d == priority1_s && priority2[d] < priority2_s);
            });
            uint32_t initial = parlay::reduce(contrib);
            if (initial == 0) { insert(s); }
            return Counter(initial);
        });
        return;
    }
    constexpr size_t SAMPLE_K = 32768;
    constexpr size_t NUM_BUCKETS = 256;
    size_t sample_n = std::min(SAMPLE_K, n);
    auto samples = parlay::sequence<uint32_t>::from_function(sample_n, [&](size_t i) {
        size_t v = parlay::hash64(i) % n;
        return priority1[v];
    });
    samples = parlay::sort(samples);
    auto cut = parlay::sequence<uint32_t>(NUM_BUCKETS - 1);
    for (size_t b = 1; b < NUM_BUCKETS; ++b) {
        size_t idx = (size_t)((double)b / NUM_BUCKETS * sample_n);
        if (idx >= sample_n) idx = sample_n - 1;
        cut[b - 1] = samples[idx];
    }
    auto small_priority = parlay::sequence<uint8_t>(n);
    parlay::parallel_for(0, n, [&](size_t i) {
        uint32_t p = priority1[i];
        size_t b = parlay::internal::binary_search(
            cut,
            [&](uint32_t x) { return x <= p; }
        );
        if (b > 255) b = 255;
        small_priority[i] = static_cast<uint8_t>(b);
    });
    counters = parlay::sequence<Counter>::from_function(n, [&](size_t s) {
        uint8_t small_priority_s = small_priority[s];
        size_t start = G.offsets[s];
        size_t end   = G.offsets[s + 1];
        size_t deg   = end - start;
        auto contrib = parlay::delayed_seq<size_t>(deg, [&](size_t j) {
            uint32_t d = G.edges[start + j].v;
            uint8_t small_priority_d = small_priority[d];
            if (small_priority_d != small_priority_s) {
                return (small_priority_d < small_priority_s);
            }
            return (priority1[d] < priority1[s] ||
                (priority1[d] == priority1[s] && priority2[d] < priority2[s]));
        });
        uint32_t initial = parlay::reduce(contrib);
        if (initial == 0) { insert(s); }
        return Counter(initial);
    });
}