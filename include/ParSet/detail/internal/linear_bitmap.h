// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <utility>
#include <algorithm>
#include <parlay/sequence.h>

namespace ParSet { namespace internal {

struct LinearBitmap {
    size_t n;
    size_t W;
    parlay::sequence<uint64_t> bitmap;
    LinearBitmap(size_t _n, bool init_value = 0): n(_n), W((n + 63) / 64) {
        if (init_value) {
            bitmap = parlay::sequence<uint64_t>((n + 63) >> 6, UINT64_MAX);
            if (n & 63) bitmap.back() = (1ULL << (n & 63)) - 1;
        }
        else {
            bitmap = parlay::sequence<uint64_t>((n + 63) >> 6, 0);
        }
    }
    inline bool contains(size_t i) { return ((bitmap[i >> 6] >> (i & 63)) & 1ULL); }
    inline void insert(size_t i) { __atomic_fetch_or(&bitmap[i >> 6], (1ULL << (i & 63)), __ATOMIC_RELAXED); }
    inline void remove(size_t i) { __atomic_fetch_and(&bitmap[i >> 6], ~(1ULL << (i & 63)), __ATOMIC_RELAXED); }
    inline void seq_insert(size_t i) { bitmap[i >> 6] |= (1ULL << (i & 63)); }
    inline void seq_remove(size_t i) { bitmap[i >> 6] &= ~(1ULL << (i & 63)); }
    inline bool try_insert(size_t i) {
        uint64_t mask = 1ULL << (i & 63);
        uint64_t old = __atomic_fetch_or(&bitmap[i >> 6], mask, __ATOMIC_RELAXED);
        return !(old & mask);
    }
    inline bool try_remove(size_t i) {
        uint64_t mask = 1ULL << (i & 63);
        uint64_t old = __atomic_fetch_and(&bitmap[i >> 6], ~mask, __ATOMIC_RELAXED);
        return (old & mask);
    }
};

}} // namespace internal & ParSet
