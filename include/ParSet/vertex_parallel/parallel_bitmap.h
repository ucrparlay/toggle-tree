#pragma once
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
    parlay::sequence<parlay::sequence<uint64_t>> augval;
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
    
    template <bool Write, bool Size, uint64_t Identity, class F, class Combine>
    inline uint64_t reduce(int layer, uint64_t base, F&& f, Combine&& combine) {
        uint64_t mask = bitmap[layer][idx(layer, base)];
        uint64_t best = Identity;
        if (layer == 4) {
            for(size_t i=0; i<__builtin_popcountll(mask); i++){
                uint64_t childbase = base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, mask)) << off(5));
                uint64_t m = bitmap[5][idx(5, childbase)];
                uint64_t b;
                if constexpr (Size) { b = __builtin_popcountll(m); }
                else {
                    b = Identity;
                    for (size_t j=0; j<__builtin_popcountll(m); j++) {
                        b = combine(b, f(childbase+__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, m))));
                    }
                }
                if constexpr (Write) augval[5][idx(5, childbase)] = b;
                best = combine(best, b);
            }
        }
        else {
            uint64_t bests[__builtin_popcountll(mask)];
            parlay::parallel_for(0, __builtin_popcountll(mask), [&](size_t j) {
                bests[j] = reduce<Write, Size, Identity>(layer + 1, base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, mask)) << off(layer + 1)), f, combine);
            }, 1);
            for (size_t j=0; j<__builtin_popcountll(mask); j++) {
                best = combine(best, bests[j]);
            }
        }
        if constexpr (Write) augval[layer][idx(layer, base)] = best;
        return best;
    }
    template<bool Write, uint64_t Identity, class F, class Combine>
    inline uint64_t reduce(F&& f, Combine&& combine){
        if (empty()) return Identity;
        if constexpr (Write) {
            if (augval.size() == 0) {
                augval = parlay::sequence<parlay::sequence<uint64_t>>(6);
                for (int i = 5; i >= 0; i--) {
                    augval[i] = parlay::sequence<uint64_t>(bitmap[i].size(), 0);
                }
            }
        }
        return reduce<Write, false, Identity>(0, 0, f, combine);
    }
    template<bool Write>
    inline uint64_t reduce_vertex(){
        if (empty()) return 0;
        if constexpr (Write) {
            if (augval.size() == 0) {
                augval = parlay::sequence<parlay::sequence<uint64_t>>(6);
                for (int i = 5; i >= 0; i--) {
                    augval[i] = parlay::sequence<uint64_t>(bitmap[i].size(), 0);
                }
            }
        }
        return reduce<Write, true, 0>(0, 0, 
            [&] ( ) { return; },
            [&] (uint64_t a, uint64_t b) { return a+b; }
        );
    }

    template<class Dist,class Emit>
    inline uint64_t extract_min_rec(int layer, uint64_t base, Dist& dist, Emit&& emit){
        uint64_t best = INT32_MAX, newmask = 0;
        if (layer == 4) {
            for(size_t i=0; i<__builtin_popcountll(bitmap[4][idx(4,base)]); i++){
                uint64_t childbase = base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<i,bitmap[4][idx(4,base)]))<<off(5));
                if (augval[5][idx(5,childbase)] != augval[0][0]) {
                    newmask |= 1ULL<<((childbase>>off(5))&63);
                    best = std::min(best,augval[5][idx(5,childbase)]);
                    continue;
                }
                uint64_t nm=0, b=INT32_MAX, m=bitmap[5][idx(5,childbase)];
                for(size_t j=0; j<__builtin_popcountll(m); j++){
                    uint64_t v = childbase+__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,m));
                    if ((uint64_t)dist[v]==augval[0][0]) { emit(v); }
                    else nm|=1ULL<<(v&63),b=std::min(b,(uint64_t)dist[v]);
                }
                bitmap[5][idx(5,childbase)] = nm;
                augval[5][idx(5,childbase)] =b ;
                if (nm) newmask|=1ULL<<((childbase>>off(5))&63), best=std::min(best,b);
            }
        }
        else if (layer == 3) {
            for(size_t j=0; j<__builtin_popcountll(bitmap[3][idx(3,base)]); j++){
                uint64_t childbase = base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,bitmap[3][idx(3,base)]))<<off(4));
                uint64_t b = 
                    (augval[4][idx(4,childbase)] == augval[0][0]) ? 
                    extract_min_rec(4,childbase,dist,emit): 
                    augval[4][idx(4,childbase)];
                if (bitmap[4][idx(4,childbase)]) {
                    newmask |= 1ULL<<((childbase>>off(4))&63);
                    best = std::min<uint64_t>(best,b);
                }
            }
        }
        else {
            uint64_t bests[__builtin_popcountll(bitmap[layer][idx(layer,base)])];
            parlay::parallel_for(0, __builtin_popcountll(bitmap[layer][idx(layer,base)]), [&](size_t j){
                uint64_t childbase=base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,bitmap[layer][idx(layer,base)]))<<off(layer+1));
                bests[j] = 
                    (augval[layer+1][idx(layer+1,childbase)] == augval[0][0]) ? 
                    extract_min_rec(layer+1,childbase,dist,emit): 
                    augval[layer+1][idx(layer+1,childbase)];
            },1);
            for(size_t j=0; j<__builtin_popcountll(bitmap[layer][idx(layer,base)]); j++){
                uint64_t childbase=base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,bitmap[layer][idx(layer,base)]))<<off(layer+1));
                if (bitmap[layer+1][idx(layer+1,childbase)]) {
                    newmask |= 1ULL<<((childbase>>off(layer+1))&63);
                    best = std::min<uint64_t>(best,bests[j]);
                }
            }
        }
        bitmap[layer][idx(layer,base)] = newmask;
        return augval[layer][idx(layer,base)] = best;
    }

    template<class Dist,class Emit>
    inline uint64_t extract_min(Dist& dist,Emit&& emit){
        if(empty()) return UINT64_MAX;
        extract_min_rec(0, 0, dist, emit);
        return augval[0][0];
    }

    template<class Dist>
    inline uint64_t repair_rec(int layer, uint64_t base, ParallelBitmap& active, Dist& dist){
        uint64_t best = INT32_MAX;
        if (layer == 4) {
            for(size_t i=0; i<__builtin_popcountll(bitmap[4][idx(4,base)]); i++){
                uint64_t childbase = base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<i,bitmap[4][idx(4,base)]))<<off(5));
                uint64_t b = INT32_MAX, m = bitmap[5][idx(5,childbase)];
                for(size_t j=0; j<__builtin_popcountll(m); j++) b = std::min<uint64_t>(b, (uint32_t)dist[childbase+__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,m))]);
                active.augval[5][idx(5,childbase)] = std::min<uint64_t>(active.augval[5][idx(5,childbase)], b);
                best = std::min<uint64_t>(best, active.augval[5][idx(5,childbase)]);
                bitmap[5][idx(5,childbase)] = 0;
            }
        }
        else {
            uint64_t bests[__builtin_popcountll(bitmap[layer][idx(layer,base)])];
            parlay::parallel_for(0, __builtin_popcountll(bitmap[layer][idx(layer,base)]), [&](size_t j){
                uint64_t childbase = base+(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL<<j,bitmap[layer][idx(layer,base)]))<<off(layer+1));
                bests[j] = repair_rec(layer+1, childbase, active, dist);
            }, 1);
            for(size_t j=0; j<__builtin_popcountll(bitmap[layer][idx(layer,base)]); j++) best = std::min<uint64_t>(best, bests[j]);
        }
        bitmap[layer][idx(layer,base)] = 0;
        active.augval[layer][idx(layer,base)] = std::min<uint64_t>(active.augval[layer][idx(layer,base)], best);
        return active.augval[layer][idx(layer,base)];
    }

    template<class Dist>
    inline void repair(ParallelBitmap& active,Dist& dist){
        if (active.augval.size() == 0) {
            active.augval = parlay::sequence<parlay::sequence<uint64_t>>(6);
            for (int i = 5; i >= 0; i--) {
                active.augval[i] = parlay::sequence<uint64_t>(active.bitmap[i].size(), INT32_MAX);
            }
        }
        if(empty()) return;
        repair_rec(0,0,active,dist);
    }

    template<bool Remove, class Sequence>
    inline void pack(int layer, uint64_t base, uint64_t offset, Sequence& out) {
        uint64_t mask = bitmap[layer][idx(layer, base)];
        if constexpr (Remove) { bitmap[layer][idx(layer,base)] = 0; }
        if (layer == 4) {
            for(size_t i=0; i<__builtin_popcountll(mask); i++){
                uint64_t childbase = base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, mask)) << off(5));
                uint64_t m = bitmap[5][idx(5, childbase)];
                if constexpr (Remove) { bitmap[5][idx(5,childbase)] = 0; }
                for (size_t j=0; j<__builtin_popcountll(m); j++) {
                    out[offset++] = childbase + __builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, m));
                }
            }
        }
        else {
            uint64_t offsets[__builtin_popcountll(mask)]; offsets[0] = offset;
            for (size_t j = 1; j < __builtin_popcountll(mask); j++) {
                uint64_t prev_base = base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << (j - 1), mask)) << off(layer + 1));
                offsets[j] = offsets[j - 1] + augval[layer + 1][idx(layer + 1, prev_base)];
            }
            parlay::parallel_for(0, __builtin_popcountll(mask), [&](size_t j) {
                pack<Remove>(layer + 1, base + (__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << j, mask)) << off(layer + 1)), offsets[j], out);
            }, 1);
        }
    }
    template<bool Remove, class T>
    inline parlay::sequence<T> pack() {
        if (empty()) return {};
        uint64_t total = reduce_vertex<true>();
        parlay::sequence<T> out(total);
        pack<Remove>(0, 0, 0, out);
        return out;
    }
    template<bool Remove, class Sequence>
    inline size_t pack_into(Sequence& out) {
        if (empty()) return 0;
        uint64_t total = reduce_vertex<true>();
        pack<Remove>(0, 0, 0, out);
        return total;
    }

    template<class F>
    inline void pop(size_t k, F&& f){
        if (!k || empty()) return;
        static thread_local uint64_t rnd = 0x12345678u;
        parlay::parallel_for(0,k,[&](int){
            while (true) {
                if (empty()) return;
                uint64_t base = 0, m = 0;
                for(int layer=0; layer<=5; layer++){
                    m = __atomic_load_n(&bitmap[layer][idx(layer,base)], __ATOMIC_RELAXED);
                    if (!m) break;
                    rnd = rnd * 1664525u + 1013904223u;
                    uint64_t x = rnd & 63;
                    base += ((__builtin_ctzll((m>>x)|(m<<((-x)&63)))+x) & 63)<<off(layer+1);
                }
                if (!m) continue;
                if (try_remove(base)) { f(base); return; }
            }
        });
    }
};

}} // namespace internal & ParSet

    /*struct Node {int layer; uint64_t base;};
        constexpr uint64_t CAPM = 127;
        Node nodes[CAPM+1], leaves[CAPM+1]; nodes[0] = {0, 0};
        size_t head = 0, tail = 1, leaves_n = 0;
        for (; head != tail && ((tail - head) & CAPM) + leaves_n < CAPM - 63; head = (head + 1) & CAPM) {
            if (nodes[head].layer == 4) { 
                for (uint64_t mask = bitmap[nodes[head].layer][idx(nodes[head].layer, nodes[head].base)]; mask; mask &= mask - 1) {
                    leaves[leaves_n].layer = nodes[head].layer + 1;
                    leaves[leaves_n].base = nodes[head].base + (__builtin_ctzll(mask) << off(nodes[head].layer + 1));
                    leaves_n++;
                }
                if constexpr(Remove) bitmap[nodes[head].layer][idx(nodes[head].layer, nodes[head].base)] = 0;
            }
            else {
                for (uint64_t mask = bitmap[nodes[head].layer][idx(nodes[head].layer, nodes[head].base)]; mask; mask &= mask - 1) {
                    nodes[tail].layer = nodes[head].layer + 1;
                    nodes[tail].base = nodes[head].base + (__builtin_ctzll(mask) << off(nodes[head].layer + 1));
                    tail = (tail + 1) & CAPM;
                }
                if constexpr(Remove) bitmap[nodes[head].layer][idx(nodes[head].layer, nodes[head].base)] = 0;
            }
        }
        while (head != tail) {
            leaves[leaves_n++] = nodes[head];
            head = (head + 1) & CAPM;
        }
        parlay::parallel_for(0, leaves_n, [&](size_t i) {
            for_each<Remove, Gran>(leaves[i].layer, leaves[i].base, f);
        }, 1);
        
        
        
        
        */