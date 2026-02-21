#pragma once
#include <cstdint>
#include <cstdlib>
#include <climits>
#include "../scheduler/parlay/sequence.h"

struct Bitmap {
    size_t n;
    size_t W;
    parlay::sequence<uint64_t> bitmap;
    Bitmap(size_t _n, bool init_value = 0): n(_n), W((n + 63) / 64) {
        if (init_value) {
            bitmap = parlay::sequence<uint64_t>((n + 63) >> 6, UINT64_MAX);
            if (n & 63) bitmap.back() = (1ULL << (n & 63)) - 1;
        }
        else {
            bitmap = parlay::sequence<uint64_t>((n + 63) >> 6, 0);
        }
    }
    inline bool is_true(size_t i) { return ((bitmap[i >> 6] >> (i & 63)) & 1ULL); }
    inline bool is_false(size_t i) { return !((bitmap[i >> 6] >> (i & 63)) & 1ULL); }
    inline bool mark_true(size_t i) {
        uint64_t mask = 1ULL << (i & 63);
        uint64_t old = __atomic_fetch_or(&bitmap[i >> 6], mask, __ATOMIC_RELAXED);
        return !(old & mask);
    }
    inline void mark_false(size_t i) { __atomic_fetch_and(&bitmap[i >> 6], ~(1ULL << (i & 63)), __ATOMIC_RELAXED); }
    inline void seq_mark_true(size_t i) { bitmap[i >> 6] |= (1ULL << (i & 63)); }
    inline void seq_mark_false(size_t i) { bitmap[i >> 6] &= ~(1ULL << (i & 63)); }
    template <class F>
    inline void parallel_do(F&& f) {
        static const size_t WIDTH = 512;
        size_t blocks = (n + WIDTH - 1) / WIDTH;
        parlay::parallel_for(0, blocks, [&](size_t bi) {
            size_t l = bi * WIDTH;
            size_t r = std::min(l + WIDTH, n);
            size_t wl = l >> 6;
            size_t wr = (r + 63) >> 6;
            for (size_t w = wl; w < wr; ++w) {
                if (bitmap[w] == 0) continue;
                for (uint64_t inv = bitmap[w]; inv; inv &= inv - 1) {
                    f((w << 6) + __builtin_ctzll(inv));
                }
            }
        });
    }
};