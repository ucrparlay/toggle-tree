#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <utility>
#include <algorithm>
#include <parlay/sequence.h>

namespace ParSet {

template <typename F>
inline void adaptive_for(
    size_t start,
    size_t end,
    F&& f,
    size_t threshold = 256,
    size_t granularity = 128
) {
    if (end - start < threshold) {
        for (size_t i = start; i < end; ++i) {
            f(i);
        }
    } 
    else {
        parlay::parallel_for(start, end, std::forward<F>(f), granularity);
    }
}

template <class Graph, class F>
inline uint32_t adaptive_sum(Graph& G, uint32_t s, size_t l, size_t r, F&& f) {
    if (r - l < 256) {
        uint32_t cnt = 0;
        for (size_t j = l; j < r; j++) {
            uint32_t d = G.edges[j].v;
            cnt += f(d);
        }
        return cnt;
    }
    size_t m = ((l + r) >> 5) << 4;
    uint32_t left, right;
    parlay::parallel_do(
        [&]() { left  = adaptive_sum(G, s, l, m, f); },
        [&]() { right = adaptive_sum(G, s, m, r, f); }
    );
    return left + right;
}

template <class Graph, class F>
inline uint32_t adaptive_min(Graph& G, uint32_t s, size_t l, size_t r, F&& f) {
    if (r - l < 256) {
        uint32_t result = UINT32_MAX;
        for (size_t j = l; j < r; j++) {
            uint32_t d = G.edges[j].v;
            uint32_t fd = f(d);
            if (result > fd) result = fd;
        }
        return result;
    }
    size_t m = ((l + r) >> 5) << 4;
    uint32_t left, right;
    parlay::parallel_do(
        [&]() { left  = adaptive_min(G, s, l, m, f); },
        [&]() { right = adaptive_min(G, s, m, r, f); }
    );
    return std::min(left, right);
}

template <class Graph, class F>
inline bool adaptive_exist(Graph& G, uint32_t s, size_t l, size_t r, F&& f) {
    if (r - l < 256) {
        for (size_t j = l; j < r; j++) {
            uint32_t d = G.edges[j].v;
            if (f(d)) return true;
        }
        return false;
    }
    size_t m = ((l + r) >> 5) << 4;
    bool left, right;
    parlay::parallel_do(
        [&]() { left  = adaptive_exist(G, s, l, m, f); },
        [&]() { right = adaptive_exist(G, s, m, r, f); }
    );
    return left || right;
}

} // namespace ParSet
