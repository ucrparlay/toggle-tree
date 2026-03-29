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
    struct Node { uint64_t bitmap; uint64_t dirty; T augval; };
    Sequence& sequence;
    std::vector<Node> tree[6];
    static constexpr uint64_t off(int32_t i) noexcept { return 6*(6-i); }
    static constexpr uint64_t idx(int32_t i, uint64_t base) noexcept { return base >> off(i); }
    static constexpr uint64_t fnd(int32_t i, uint64_t mask) noexcept { return __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, mask)); }

    inline T repair_rec(int32_t layer, uint64_t base) {
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
                bests[i] = repair_rec(layer + 1, base + (fnd(i, d) << off(layer + 1)));
            }, 1);
            for (int32_t i = 0; i < __builtin_popcountll(d); i++) { best = std::min(best, bests[i]); }
        }
        return tree[layer][idx(layer, base)].augval = best;
    }

    template<class F>
    inline T extract_min_rec(int32_t layer, uint64_t base, T threshold, F&& f){
        uint64_t mask = tree[layer][idx(layer, base)].bitmap;
        uint64_t newmask = 0;
        T best = std::numeric_limits<T>::max();
        if (layer == 5) {
            for (int32_t i = 0; i < __builtin_popcountll(mask); i++) {
                uint64_t v = base + fnd(i, mask);
                if (sequence[v] <= threshold) { f(v); }
                else { best = std::min(best, sequence[v]); newmask |= 1ULL << (v & 63); }
            }
        }
        else {
            T bests[__builtin_popcountll(mask)]; int32_t bests_size = 0;
            uint64_t bases[__builtin_popcountll(mask)];
            for (int32_t i=0; i < __builtin_popcountll(mask); i++) {
                uint64_t childbase = base + (fnd(i, mask) << off(layer + 1));
                T childaugval = tree[layer + 1][idx(layer + 1, childbase)].augval;
                if (childaugval <= threshold) { bases[bests_size] = childbase; bests_size++; }
                else { best = std::min(best, childaugval); newmask |= 1ULL << fnd(i, mask); }
            }
            parlay::parallel_for(0, bests_size, [&](int32_t i) {
                bests[i] = extract_min_rec(layer + 1, bases[i], threshold, f);
            }, 1);
            for (int32_t i = 0; i < bests_size; i++) {
                best = std::min(best, bests[i]);
                if (tree[layer + 1][idx(layer + 1, bases[i])].bitmap) {
                    newmask |= 1ULL << ((bases[i] >> off(layer + 1)) & 63);
                }
            }
        }
        tree[layer][idx(layer, base)].bitmap = newmask;
        return tree[layer][idx(layer, base)].augval = best;
    }
    
public:
    TournamentTree(Sequence& _sequence): sequence(_sequence) {
        uint64_t length = sequence.size();
        for (int32_t i=5; i>=0; i--) {
            tree[i] = std::vector<Node>((length + 63) >> 6, Node{0, 0, std::numeric_limits<T>::max()});
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
    template <class F> inline T extract(F&& f) {
        T min_value = tree[0][0].dirty ? repair_rec(0, 0) : tree[0][0].augval;
        if (!tree[0][0].bitmap) return std::numeric_limits<T>::max();
        extract_min_rec(0, 0, min_value, f);
        return min_value;
    }
};

}} // namespace internal & ParSet
