#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <utility>
#include <algorithm>
//#include <vector>
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
        if (bitmap[5][v>>6] & bit5) return false;
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
        if (!(bitmap[5][v>>6] & bit5)) return false;
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
                //if constexpr (Write) augval[5][idx(5, childbase)] = b;
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
                augval = parlay::sequence<parlay::sequence<uint64_t>>(5/*6*/);
                for (int i = 4/*5*/; i >= 0; i--) {
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
                augval = parlay::sequence<parlay::sequence<uint64_t>>(5/*6*/);
                for (int i = 4/*5*/; i >= 0; i--) {
                    augval[i] = parlay::sequence<uint64_t>(bitmap[i].size(), 0);
                }
            }
        }
        return reduce<Write, true, 0>(0, 0, 
            [&] ( ) { return; },
            [&] (uint64_t a, uint64_t b) { return a+b; }
        );
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
            /*template<bool Write, class F, class Combine>
    inline uint64_t reduce(int layer, uint64_t base, uint64_t mask, F&& f, Combine&& combine){
        if (layer >= 4){
            uint64_t best = 0;
            if(layer == 5){
                for(; mask; mask &= mask-1){
                    uint64_t t = base+__builtin_ctzll(mask);
                    best = combine(best, f(t));
                }
            } 
            else {
                for(; mask; mask &= mask-1){
                    uint64_t child_base = base + (__builtin_ctzll(mask)<<off(layer+1));
                    auto r = reduce<Write>(layer+1, child_base, bitmap[layer+1][idx(layer+1,child_base)], f, combine);
                    best = combine(best, r);
                }
            }
            if constexpr (Write) augval[layer][idx(layer, base)] = best;
            return best;
        }
        if((mask^mask&-mask)==0){
            if constexpr (Write) {
                return augval[layer][idx(layer, base)] = reduce<Write>(layer+1, base + (__builtin_ctzll(mask)<<off(layer+1)), bitmap[layer+1][idx(layer+1,base + (__builtin_ctzll(mask)<<off(layer+1)))], f, combine);
            }
            else{
                return reduce<Write>(layer+1, base + (__builtin_ctzll(mask)<<off(layer+1)), bitmap[layer+1][idx(layer+1,base + (__builtin_ctzll(mask)<<off(layer+1)))], f, combine);
            }
        }
        uint64_t l, r;
        parlay::parallel_do(
            [&](){ l = reduce<Write>(layer, base, mask^mask&-mask, f, combine); },
            [&](){ r = reduce<Write>(layer+1, base + (__builtin_ctzll(mask)<<off(layer+1)), bitmap[layer+1][idx(layer+1,base + (__builtin_ctzll(mask)<<off(layer+1)))], f, combine); }
        );
        if constexpr (Write) { return augval[layer][idx(layer, base)] = combine(l, r); }
        else { return combine(l, r); }
    }
    template <class F, class Select>
    inline void select(int layer, uint64_t base, uint64_t mask, uint64_t best, F&& f, Select&& sel) {
        if (layer >= 4) {
            if (layer == 5) {
                for (; mask != 0; mask &= mask - 1) {
                    uint64_t i = base + __builtin_ctzll(mask);
                    if (f(i) == best) sel(i);
                }
            }
            else {
                for (; mask != 0; mask &= mask - 1) {
                    uint64_t child_base = base + (__builtin_ctzll(mask) << off(layer + 1));
                    if (augval[layer+1][idx(layer+1, child_base)] == best)
                    select(layer + 1, child_base, bitmap[layer + 1][idx(layer + 1, child_base)], best, f, sel); 
                }
            }
            return;
        }
        if ((mask ^ mask & -mask) == 0) {
            if (augval[layer+1][idx(layer+1, base + (__builtin_ctzll(mask & -mask) << off(layer+1)))] == best)
            select(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)), bitmap[layer + 1][idx(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)))], best, f, sel);
            return;
        }
        parlay::parallel_do(
            [&]() { select(layer, base, mask ^ mask & -mask, best, f, sel); },
            [&]() { 
                if (augval[layer+1][idx(layer+1, base + (__builtin_ctzll(mask & -mask) << off(layer+1)))] == best)
                select(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)), bitmap[layer + 1][idx(layer + 1, base + (__builtin_ctzll(mask & -mask) << off(layer + 1)))], best, f, sel); 
            }
        );
    }
    template <class F, class Select>
    inline void select(F&& f, Select&& sel) {
        if (empty()) return;
        uint64_t best = augval[0][0];
        if (best == 0) return;
        select(0, 0, bitmap[0][0], best, f, sel);
    }    
    
    template<class Graph, class Cond, class Update>
    inline void edgemap(Graph& G, Cond&& cond, Update&& update) {
        if (empty()) return;
        uint64_t total = reduce<true>(
            [&] (size_t s) { return G.offsets[s+1] - G.offsets[s]; },
            [&] (uint64_t l, uint64_t r) { return l + r; }
        );
        if (!total) return;
        const uint64_t block_size = 1024, num_blocks = (total + block_size - 1) / block_size;
        parlay::parallel_for(0, num_blocks, [&](size_t block_idx) {
            uint64_t L = block_idx * block_size, need = std::min(block_size, total - L), done = 0;
            uint64_t B[6], M[6], offv = 0; size_t s = 0;

            auto seek = [&](uint64_t k) {
                uint64_t base = 0, mask = bitmap[0][0], rem = k;
                for (int layer = 0; layer < 5; layer++) {
                    uint64_t mm = mask;
                    while (true) {
                        uint64_t bit = mm & -mm;
                        uint64_t child_base = base + (__builtin_ctzll(bit) << off(layer + 1));
                        uint64_t w = augval[layer + 1][idx(layer + 1, child_base)];
                        if (rem < w) { B[layer] = base; M[layer] = mm; base = child_base; mask = bitmap[layer + 1][idx(layer + 1, child_base)]; break; }
                        rem -= w; mm ^= bit;
                    }
                }
                B[5] = base;
                uint64_t mm = mask;
                while (true) {
                    uint64_t bit = mm & -mm;
                    uint64_t v = base + __builtin_ctzll(bit);
                    uint64_t deg = G.offsets[v+1] - G.offsets[v];
                    if (rem < deg) { M[5] = mm; s = v; offv = rem; return; }
                    rem -= deg; mm ^= bit;
                }
            };

            auto next_vertex = [&]() -> bool {
                M[5] &= M[5] - 1;
                if (M[5]) { s = B[5] + __builtin_ctzll(M[5]); offv = 0; return true; }
                for (int layer = 4; layer >= 0; layer--) {
                    M[layer] &= M[layer] - 1;
                    if (!M[layer]) continue;
                    uint64_t child_base = B[layer] + (__builtin_ctzll(M[layer]) << off(layer + 1));
                    for (int t = layer + 1; t <= 5; t++) {
                        B[t] = child_base;
                        uint64_t m = bitmap[t][idx(t, child_base)];
                        M[t] = m;
                        if (t == 5) break;
                        child_base += (__builtin_ctzll(m) << off(t + 1));
                    }
                    s = B[5] + __builtin_ctzll(M[5]); offv = 0; return true;
                }
                return false;
            };

            seek(L);
            while (done < need) {
                uint64_t deg = G.offsets[s+1] - G.offsets[s];
                uint64_t take = std::min(deg - offv, need - done);
                uint64_t e0 = G.offsets[s] + offv;
                for (uint64_t i = 0; i < take; i++) {
                    if (cond(G.edges[e0 + i].v)) update(s, G.edges[e0 + i].v);
                }
                done += take; offv += take;
                if (done == need) break;
                if (offv == deg && !next_vertex()) break;
            }
        }, 1);
    }
    */