// SPDX-License-Identifier: MIT
#pragma once
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <utility>
#include <algorithm>
#include <parlay/sequence.h>

namespace ParSet { namespace internal {

struct ParallelBitmap {
    parlay::sequence<parlay::sequence<uint64_t>> bitmap;
    static constexpr uint64_t off(int i) noexcept { return 6*(6-i); } 
    static constexpr uint64_t idx(int i, uint64_t base) noexcept { return base >> off(i); } 
    ParallelBitmap(size_t n, bool init_value) {
        bitmap = parlay::sequence<parlay::sequence<uint64_t>>(6);
        uint64_t length = n;
        if (init_value) {
            for (int i = 5; i >= 0; i--) {
                bitmap[i] = parlay::sequence<uint64_t>((length + 63) >> 6, UINT64_MAX);
                if (length & 63) bitmap[i].back() = (1ULL << (length & 63)) - 1;
                length = (length + 63) >> 6;
            }
        } else {
            for (int i = 5; i >= 0; i--) {
                bitmap[i] = parlay::sequence<uint64_t>((length + 63) >> 6, 0);
                length = (length + 63) >> 6;
            }
        }
    }

    inline bool empty() const noexcept { return !bitmap[0][0]; }
    inline bool contains(size_t v) const noexcept { return (bitmap[5][v>>6]>>(v&63)) & 1ULL; }
    inline void insert(size_t v) noexcept {
        for (int i=5; i>=0; i--) { if (__atomic_fetch_or(&bitmap[i][v>>off(i)], (1ULL<<((v>>off(i+1)) & 63)), __ATOMIC_RELAXED)) return; }
    }
    inline void remove(size_t v) noexcept {
        for (int i=5;i>=0;i--) {
            if (__atomic_fetch_and(&bitmap[i][v >> off(i)], ~(1ULL << ((v >> off(i+1)) & 63)), __ATOMIC_RELAXED) & ~(1ULL << ((v >> off(i+1)) & 63))) return;
        }
    }
    inline bool try_insert(size_t v) noexcept {
        uint64_t bit5 = 1ULL << (v & 63);
        uint64_t old5 = __atomic_fetch_or(&bitmap[5][v>>6], bit5, __ATOMIC_RELAXED);
        if (old5 & bit5) return false;
        if (old5) return true;
        for (int i=4;i>=0;i--) {
            if (__atomic_fetch_or(&bitmap[i][v>>off(i)], (1ULL<<((v>>off(i+1))&63)), __ATOMIC_RELAXED) & (1ULL<<((v>>off(i+1))&63))) return true;
        }
        return true;
    }
    inline bool try_remove(size_t v) noexcept {
        uint64_t bit5 = 1ULL << (v & 63);
        uint64_t old5 = __atomic_fetch_and(&bitmap[5][v >> 6], ~bit5, __ATOMIC_RELAXED);
        if (!(old5 & bit5)) return false;
        if (old5 & ~bit5) return true;
        for (int i=4;i>=0;i--) {
            if (__atomic_fetch_and(&bitmap[i][v >> off(i)], ~(1ULL << ((v >> off(i+1)) & 63)), __ATOMIC_RELAXED) & ~(1ULL << ((v >> off(i+1)) & 63))) return true;
        }
        return true;
    }
    
    template <bool Remove, size_t Gran, class F>
    inline void for_each(int layer, uint64_t base, F&& f) {
        uint64_t mask = bitmap[layer][idx(layer, base)];
        if constexpr (Remove) bitmap[layer][idx(layer, base)] = 0;
        if (layer == 5) {
            parlay::parallel_for(0, __builtin_popcountll(mask), [&](int j) {
                f(base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, mask)) << off(layer + 1)));
            }, Gran);
        }
        else {
            parlay::parallel_for(0, __builtin_popcountll(mask), [&](int j) {
                for_each<Remove, Gran>(layer + 1, base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, mask)) << off(layer + 1)), f);
            }, 1);
        }
    }
    template <bool Remove, size_t Gran, class F>
    inline void for_each(F&& f) {
        if (empty()) return;
        for_each<Remove, Gran>(0, 0, f);
    }

    template<class T, auto Identity, class F, class Combine>
    inline T reduce(int layer, uint64_t base, F&& f, Combine&& combine){
        uint64_t mask = bitmap[layer][idx(layer,base)];
        T best = Identity;
        if (layer==4) {
            for(size_t i=0;i<__builtin_popcountll(mask);i++){
                uint64_t childbase = base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<i,mask))<<off(5));
                uint64_t m = bitmap[5][idx(5,childbase)];
                T b = Identity;
                for(size_t j=0;j<__builtin_popcountll(m);j++) b = combine(b,f(childbase+__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,m))));
                best = combine(best,b);
            }
        }
        else{
            T bests[__builtin_popcountll(mask)];
            parlay::parallel_for(0, __builtin_popcountll(mask), [&](size_t j){
                bests[j] = reduce<T, Identity>(layer+1,base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,mask))<<off(layer+1)),f,combine);
            },1);
            for(size_t j=0;j<__builtin_popcountll(mask);j++) best=combine(best,bests[j]);
        }
        return best;
    }
    template<class T, auto Identity, class F, class Combine>
    inline T reduce(F&& f, Combine&& combine){
        if (empty()) return Identity;
        return reduce<T, Identity>(0, 0, f, combine);
    }

};

}} // namespace internal & ParSet
