// SPDX-License-Identifier: MIT
#pragma once
#include <vector>
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <utility>
#include <algorithm>
#include <parlay/sequence.h>
#include "../utils/atomic_operation.h"

namespace ParSet { namespace internal {

template <class Sequence>
struct TournamentTree {

private:
    using T = typename Sequence::value_type;
    struct Node { T augval; uint64_t bitmap, dirty; };
    Sequence& sequence;
    std::vector<Node> tree[6];
    static constexpr uint64_t off(int8_t i) noexcept { return 6*(6-i); }
    static constexpr uint64_t idx(int8_t i, uint64_t base) noexcept { return base >> off(i); }

    inline T repair_rec(uint8_t layer, uint64_t base){
        Node& cur = tree[layer][idx(layer, base)];
        uint64_t d = cur.dirty;
        cur.dirty = 0;
        T best = cur.augval;

        if (layer == 5) {
            cur.bitmap |= d;
            for (size_t j = 0; j < __builtin_popcountll(d); j++) {
                uint64_t v = base + __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, d));
                best = std::min(best, sequence[v]);
            }
            return cur.augval = best;
        }

        if (layer == 4) {
            for (size_t i = 0; i < __builtin_popcountll(d); i++) {
                uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, d));
                uint64_t childbase = base + (bit << off(5));
                T b = repair_rec(5, childbase);
                if (tree[5][idx(5, childbase)].bitmap) {
                    cur.bitmap |= 1ULL << bit;
                    best = std::min(best, b);
                }
            }
            return cur.augval = best;
        }

        T bests[__builtin_popcountll(d)];
        parlay::parallel_for(0, __builtin_popcountll(d), [&](size_t j) {
            uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, d));
            uint64_t childbase = base + (bit << off(layer + 1));
            bests[j] = repair_rec(layer + 1, childbase);
        }, 1);
        for (size_t j = 0; j < __builtin_popcountll(d); j++) {
            uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, d));
            uint64_t childbase = base + (bit << off(layer + 1));
            if (tree[layer + 1][idx(layer + 1, childbase)].bitmap) {
                cur.bitmap |= 1ULL << bit;
                best = std::min(best, bests[j]);
            }
        }
        return cur.augval = best;
    }

    template<class F>
    inline T extract_min_rec(uint8_t layer, uint64_t base, T threshold, F&& f){
        Node& cur = tree[layer][idx(layer, base)];
        uint64_t newmask = 0;
        T best = std::numeric_limits<T>::max();

        if (layer == 4) {
            uint64_t m4 = cur.bitmap;
            for (size_t i = 0; i < __builtin_popcountll(m4); i++) {
                uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, m4));
                uint64_t childbase = base + (bit << off(5));
                Node& ch = tree[5][idx(5, childbase)];
                if (ch.augval != threshold) {
                    newmask |= 1ULL << bit;
                    best = std::min(best, ch.augval);
                    continue;
                }
                uint64_t nm = 0;
                T b = std::numeric_limits<T>::max();
                uint64_t m = ch.bitmap;
                for (size_t j = 0; j < __builtin_popcountll(m); j++) {
                    uint64_t v = childbase + __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, m));
                    if (sequence[v] == threshold) f(v);
                    else { nm |= 1ULL << (v & 63); b = std::min(b, sequence[v]); }
                }
                ch.bitmap = nm;
                ch.augval = b;
                if (nm) { newmask |= 1ULL << bit; best = std::min(best, b); }
            }
            cur.bitmap = newmask;
            return cur.augval = best;
        }

        if (layer == 3) {
            uint64_t m3 = cur.bitmap;
            for (size_t j = 0; j < __builtin_popcountll(m3); j++) {
                uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, m3));
                uint64_t childbase = base + (bit << off(4));
                Node& ch = tree[4][idx(4, childbase)];
                T b = (ch.augval == threshold) ? extract_min_rec(4, childbase, threshold, f) : ch.augval;
                if (ch.bitmap) {
                    newmask |= 1ULL << bit;
                    best = std::min(best, b);
                }
            }
            cur.bitmap = newmask;
            return cur.augval = best;
        }

        uint64_t m = cur.bitmap;
        T bests[__builtin_popcountll(m)];
        parlay::parallel_for(0, __builtin_popcountll(m), [&](size_t j) {
            uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, m));
            uint64_t childbase = base + (bit << off(layer + 1));
            Node& ch = tree[layer + 1][idx(layer + 1, childbase)];
            bests[j] = (ch.augval == threshold) ? extract_min_rec(layer + 1, childbase, threshold, f) : ch.augval;
        }, 1);
        for (size_t j = 0; j < __builtin_popcountll(m); j++) {
            uint64_t bit = __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, m));
            uint64_t childbase = base + (bit << off(layer + 1));
            if (tree[layer + 1][idx(layer + 1, childbase)].bitmap) {
                newmask |= 1ULL << bit;
                best = std::min(best, bests[j]);
            }
        }
        cur.bitmap = newmask;
        return cur.augval = best;
    }

public:
    TournamentTree(Sequence& _sequence): sequence(_sequence) {
        uint64_t length = sequence.size();
        for (int8_t i = 5; i >= 0; i--) {
            tree[i] = std::vector<Node>((length + 63) >> 6, Node{std::numeric_limits<T>::max(), 0, 0});
            length = (length + 63) >> 6;
        }
    }

    inline void update(size_t idx0, T value) {
        if (write_min(sequence[idx0], value)) {
            for (int8_t i = 5; i >= 0; i--) {
                if (__atomic_fetch_or(&tree[i][idx0 >> off(i)].dirty, (1ULL << ((idx0 >> off(i + 1)) & 63)), __ATOMIC_RELAXED)) return;
            }
        }
    }

    template <class F>
    inline T extract(F&& f) {
        T min_value = tree[0][0].dirty ? repair_rec(0, 0) : tree[0][0].augval;
        if (!tree[0][0].bitmap) return std::numeric_limits<T>::max();
        extract_min_rec(0, 0, min_value, f);
        return min_value;
    }
};

}} // namespace internal & ParSet