#pragma once
#include <cstdint>
#include <cstdlib>
#include <climits>
#include "../parlay/parallel.h"
#include "../parlay/sequence.h"
#include "../parlay/utilities.h"

template<bool Remove>
struct ParallelBitmap {
    uint64_t n;
    int fork_depth;
    parlay::sequence<parlay::sequence<uint64_t>> bitmap;
    inline uint64_t off(int i) { return 6*(6-i); } 
    inline uint64_t idx(int i, uint64_t base) { return base >> off(i); } 
    ParallelBitmap(size_t _n, bool init_value, uint64_t _fork_depth) : n(_n), fork_depth(_fork_depth) {
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
    inline bool exist_true() { return bitmap[0][0]; }
    inline void set_fork_depth(int depth) { fork_depth = depth; }
    inline bool is_true(size_t v) { return (bitmap[5][v>>6]>>(v&63)) & 1ULL; }
    inline void mark_true(size_t v) {
        for (int i=5; i>=0; i--) { if (__atomic_fetch_or(&bitmap[i][v>>off(i)], (1ULL<<((v>>off(i+1)) & 63)), __ATOMIC_RELAXED)) return; }
    }
    inline void mark_false(size_t v) {
        for (int i=5;i>=0;i--) {
            if (__atomic_fetch_and(&bitmap[i][v >> off(i)], ~(1ULL << ((v >> off(i+1)) & 63)), __ATOMIC_RELAXED) & ~(1ULL << ((v >> off(i+1)) & 63))) return;
        }
    }
    inline bool test_mark_true(size_t v){
        uint64_t bit5 = 1ULL << (v & 63);
        if (bitmap[5][v>>6] & bit5) return false;
        uint64_t old5 = __atomic_fetch_or(&bitmap[5][v>>6], bit5, __ATOMIC_RELAXED);
        if (old5 & bit5) return false;
        if (old5) return true;
        for (int i=4;i>=0;i--) {
            if (__atomic_fetch_or(&bitmap[i][v>>off(i)], (1ULL<<((v>>off(i+1))&63)), __ATOMIC_RELAXED) & (1ULL<<((v>>off(i+1))&63))) return true;
        }
        return true;
    }
    inline bool test_mark_false(size_t v){
        uint64_t bit5 = 1ULL << (v & 63);
        if (!(bitmap[5][v>>6] & bit5)) return false;
        uint64_t old5 = __atomic_fetch_and(&bitmap[5][v >> 6], ~bit5, __ATOMIC_RELAXED);
        if (!(old5 & bit5)) return false;
        if (old5 & ~bit5) return true;
        for (int i=4;i>=0;i--) {
            if (__atomic_fetch_and(&bitmap[i][v >> off(i)], ~(1ULL << ((v >> off(i+1)) & 63)), __ATOMIC_RELAXED) & ~(1ULL << ((v >> off(i+1)) & 63))) return true;
        }
        return true;
    }
    template <class F>
    inline void visit_layer(int layer, uint64_t base, uint64_t mask, F& f) {
        if (layer >= fork_depth) {
            if (layer == 5) {
                for (; mask != 0; mask &= mask - 1) {
                    f(base + __builtin_ctzll(mask));
                }
            }
            else {
                for (; mask != 0; mask &= mask - 1) {
                    uint64_t child_base = base + (__builtin_ctzll(mask) << off(layer + 1));
                    visit_layer(layer + 1, child_base, bitmap[layer + 1][idx(layer + 1, child_base)], f ); 
                }
            }
            if constexpr (Remove) { bitmap[layer][idx(layer,base)] = 0; }
            return;
        }
        if ((mask ^ mask & -mask) == 0) {
            visit_layer(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)), bitmap[layer + 1][idx(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)))],f );
            if constexpr (Remove) bitmap[layer][idx(layer, base)] = 0;
            return;
        }
        parlay::parallel_do(
            [&]() { visit_layer(layer, base, mask ^ mask & -mask, f); },
            [&]() { visit_layer(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)), bitmap[layer + 1][idx(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)))], f ); }
        );
    }
    template <class F>
    inline void parallel_do(F&& f) {
        if (!exist_true()) return;
        visit_layer(0, 0, bitmap[0][0], f);
    }
    template <class Graph, class Source, class Cond, class Update>
    void EdgeMapSparse(Graph& G, Source&&source, Cond&& cond, Update&& update) {
        parallel_do([&](uint32_t s) {
            source(s);
            size_t start = G.offsets[s];
            size_t end   = G.offsets[s + 1];
            adaptive_for(start, end, [&](size_t j) { 
                size_t d = G.edges[j].v;
                if (cond(d)) { 
                    update(d);
                }
            });
        });
    }

    template<class F, class Combine>
    inline uint64_t reduce_layer(int layer, uint64_t base, uint64_t mask, F&& f, Combine&& combine){
        if (base >= n) return 0;
        if (layer >= 4){
            if(layer == 5){
                uint64_t best = 0;
                for(; mask; mask &= mask-1){
                    uint64_t t = base+__builtin_ctzll(mask);
                    best = combine(best, f(t));
                }
                return best;
            } else {
                uint64_t best = 0;
                for(; mask; mask &= mask-1){
                    uint64_t child_base = base + (__builtin_ctzll(mask)<<off(layer+1));
                    auto r = reduce_layer(layer+1, child_base, bitmap[layer+1][idx(layer+1,child_base)], f, combine);
                    best = combine(best, r);
                }
                return best;
            }
        }
        uint64_t child_base=base+(__builtin_ctzll(mask&-mask)<<off(layer+1));
        if((mask^mask&-mask)==0){
            return reduce_layer(layer+1, child_base, bitmap[layer+1][idx(layer+1,child_base)], f, combine);
        }
        uint64_t l, r;
        parlay::parallel_do(
            [&](){ l = reduce_layer(layer, base, mask^mask&-mask, f, combine); },
            [&](){ r = reduce_layer(layer+1, child_base, bitmap[layer+1][idx(layer+1,child_base)], f, combine); }
        );
        return combine(l, r);
    }

    template<class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){
        if (!exist_true()) return 0;
        return reduce_layer(0, 0, bitmap[0][0], f, combine);
    }
    inline uint64_t reduce_max(parlay::sequence<uint32_t>& array){
        return reduce(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (r == 0 || l > r) ? l : r; }
        );
    }
    inline uint64_t reduce_min(parlay::sequence<uint32_t>& array){
        return reduce(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
        );
    }
    inline uint64_t reduce_sum(parlay::sequence<uint32_t>& array){
        return reduce(
            [&] (size_t i) { return array[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    inline uint64_t reduce_vertex(){
        return reduce(
            [&] (size_t i) { return 1; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    template <class Graph>
    inline uint64_t reduce_edge(Graph& G){
        return reduce(
            [&] (size_t i) { return G.offsets[i+1] - G.offsets[i]; },
            [&] (uint64_t l, uint64_t r) { return (l + r); }
        );
    }
    
    inline void merge_layer(int layer, uint64_t base, uint64_t mask, ParallelBitmap<false>& other) {
        if (layer >= fork_depth - 1) {
            if (layer == 4) {
                for (; mask != 0; mask &= mask - 1) {
                    uint64_t w = (base + (__builtin_ctzll(mask) << off(5))) >> 6;
                    other.bitmap[5][w] &= ~bitmap[5][w];
                    if (!other.bitmap[5][w]) {
                        for (int i = 4; i >= 0; --i) {
                            if (__atomic_fetch_and( &other.bitmap[i][(w << 6) >> off(i)], ~(1ULL << (((w << 6) >> off(i+1)) & 63)), __ATOMIC_RELAXED) & ~(1ULL << (((w << 6) >> off(i+1)) & 63)))
                                break;
                        }
                    }
                    bitmap[5][w] = 0;
                }
            }
            else {
                for (; mask != 0; mask &= mask - 1) {
                    uint64_t child_base = base + (__builtin_ctzll(mask) << off(layer + 1));
                    merge_layer(layer + 1, child_base, bitmap[layer + 1][idx(layer + 1, child_base)], other); 
                }
            }
            bitmap[layer][idx(layer,base)] = 0;
            return;
        }
        if ((mask ^ mask & -mask) == 0) {
            merge_layer(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)), bitmap[layer + 1][idx(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)))], other);
            bitmap[layer][idx(layer, base)] = 0;
            return;
        }
        parlay::parallel_do(
            [&]() { merge_layer(layer, base, mask ^ mask & -mask, other); },
            [&]() { merge_layer(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)), bitmap[layer + 1][idx(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)))], other); }
        );
    }
    
    inline void merge_to(ParallelBitmap<false>& other) {
        if (!exist_true()) return;
        merge_layer(0, 0, bitmap[0][0], other);
    }

    template<class Other, class F>
    inline void pop(uint32_t k, Other& other, F&& f){
        if (!k || !bitmap[0][0]) return;
        static thread_local uint64_t rnd = 0x12345678u;
        parlay::parallel_for(0,k,[&](int){
            while (true) {
                if (!bitmap[0][0]) return;
                uint64_t base = 0, m = 0;
                for(int layer=0; layer<=5; layer++){
                    m = __atomic_load_n(&bitmap[layer][idx(layer,base)], __ATOMIC_RELAXED);
                    if (!m) break;
                    rnd = rnd * 1664525u + 1013904223u;
                    uint64_t x = rnd & 63;
                    base += ((__builtin_ctzll((m>>x)|(m<<((-x)&63)))+x) & 63)<<off(layer+1);
                }
                if (!m) continue;
                if (test_mark_false(base)) { other.insert(base); f(base); return; }
            }
        });
    }

    inline void pop64(uint32_t& k, parlay::sequence<uint32_t>& other){
        if (!k || !bitmap[0][0]) return;
        if (k > 64) k = 64;
        static thread_local uint64_t rnd = 0x12345678u;
        uint32_t index = 0;
        parlay::parallel_for(0,k,[&](int){
            while (true) {
                if (!bitmap[0][0]) return;
                uint64_t base = 0, m = 0;
                for(int layer=0; layer<=5; layer++){
                    m = __atomic_load_n(&bitmap[layer][idx(layer,base)], __ATOMIC_RELAXED);
                    if (!m) break;
                    rnd = rnd * 1664525u + 1013904223u;
                    uint64_t x = rnd & 63;
                    base += ((__builtin_ctzll((m>>x)|(m<<((-x)&63)))+x) & 63)<<off(layer+1);
                }
                if (!m) continue;
                if (test_mark_false(base)) { other[__atomic_fetch_add(&index,1,__ATOMIC_RELAXED)] = base; return; }
            }
        });
        for (int i=0; i<index; i++) { mark_true(other[i]); }
        k = index;
    }/*
    inline void pop64(uint32_t& k, parlay::sequence<uint32_t>& other){
        if (!k || !bitmap[0][0]) return;
        k = 1;
        for (int _i=0;_i<n;_i++) {
            if (test_mark_false(_i)) { other[0] = _i; return; }
            //if (is_true(_i)) { mark_false(_i); other[0] = _i; return; }
        }
    }*/

};