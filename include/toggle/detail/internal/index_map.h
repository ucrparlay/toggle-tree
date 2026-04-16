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

namespace toggle { namespace internal {

template <class Sequence>
struct IndexMap {
    using T = typename Sequence::value_type;
    struct Node { uint64_t bitmap; uint64_t dirty; T augval; };
    Sequence& sequence;
    parlay::sequence<parlay::sequence<Node>> tree;
    static constexpr uint64_t off(int32_t i) noexcept { return 6*(6-i); }
    static constexpr uint64_t idx(int32_t i, uint64_t base) noexcept { return base >> off(i); }
    static constexpr uint64_t fnd(int32_t i, uint64_t mask) noexcept { return __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, mask)); }

    IndexMap(Sequence& _sequence): sequence(_sequence) {
        tree = parlay::sequence<parlay::sequence<Node>>(6);
        uint64_t length = sequence.size();
        for (int32_t i=5; i>=0; i--) {
            tree[i] = parlay::sequence<Node>((length + 63) >> 6, Node{0, 0, std::numeric_limits<T>::max()});
            length = (length + 63) >> 6;
        }
    }
    inline bool update(size_t idx, T value) {
        if (write_min(sequence[idx], value)) {
            for (int32_t i=5; i>=0; i--) { if (__atomic_fetch_or(&tree[i][idx>>off(i)].dirty, (1ULL<<((idx>>off(i+1)) & 63)), __ATOMIC_RELAXED)) return true; }
            return true;
        }
        return false;
    }

    inline T repair(int32_t layer, uint64_t base) {
        uint64_t d = tree[layer][idx(layer, base)].dirty;
        tree[layer][idx(layer, base)].dirty = 0;
        tree[layer][idx(layer, base)].bitmap |= d;
        T best = tree[layer][idx(layer, base)].augval;
        if (layer == 5) {
            for (int32_t i = 0; i < __builtin_popcountll(d); i++) {
                best = std::min(best, sequence[base + fnd(i, d)]);
            }
        }
        else {
            T bests[__builtin_popcountll(d)];
            parlay::parallel_for(0, __builtin_popcountll(d), [&](uint32_t i) {
                bests[i] = repair(layer + 1, base + (fnd(i, d) << off(layer + 1)));
            }, 1);
            for (int32_t i = 0; i < __builtin_popcountll(d); i++) { best = std::min(best, bests[i]); }
        }
        return tree[layer][idx(layer, base)].augval = best;
    }
    inline T repair() {
        if (!tree[0][0].dirty) return tree[0][0].augval;
        return repair(0, 0);
    }
    
    template<class F>
    inline uint64_t extract_min(int32_t layer, uint64_t base, T threshold, F&& f){
        uint64_t mask = tree[layer][idx(layer, base)].bitmap;
        uint64_t newmask = 0;
        uint64_t cnt = 0;
        T best = std::numeric_limits<T>::max();
        if (layer == 5) {
            for (int32_t i = 0; i < __builtin_popcountll(mask); i++) {
                uint64_t v = base + fnd(i, mask);
                if (sequence[v] <= threshold) { f(v); cnt++; }
                else { best = std::min(best, sequence[v]); newmask |= 1ULL << (v & 63); }
            }
        }
        else {
            int32_t bests_size = 0;
            uint64_t bases[__builtin_popcountll(mask)];
            for (int32_t i=0; i < __builtin_popcountll(mask); i++) {
                uint64_t childbase = base + (fnd(i, mask) << off(layer + 1));
                T childaugval = tree[layer + 1][idx(layer + 1, childbase)].augval;
                if (childaugval <= threshold) { bases[bests_size] = childbase; bests_size++; }
                else { best = std::min(best, childaugval); newmask |= 1ULL << fnd(i, mask); }
            }
            T bests[bests_size];
            T cnts[bests_size];
            parlay::parallel_for(0, bests_size, [&](int32_t i) {
                cnts[i] = extract_min(layer + 1, bases[i], threshold, f);
                bests[i] = tree[layer + 1][idx(layer + 1, bases[i])].augval;
            }, 1);
            for (int32_t i = 0; i < bests_size; i++) {
                cnt += cnts[i];
                best = std::min(best, bests[i]);
                if (tree[layer + 1][idx(layer + 1, bases[i])].bitmap) {
                    newmask |= 1ULL << ((bases[i] >> off(layer + 1)) & 63);
                }
            }
        }
        tree[layer][idx(layer, base)].bitmap = newmask;
        tree[layer][idx(layer, base)].augval = best;
        return cnt;
    }
    template<class F>
    inline uint64_t extract_min(T threshold, F&& f) {
        if (!tree[0][0].bitmap) return 0;
        return extract_min(0, 0, threshold, f);
    }
};

}} // namespace internal & ParSet
