#pragma once
#include <cstdint>
#include <cstdlib>
#include <climits>
#include "../parlay/parallel.h"
#include "../parlay/sequence.h"
#include "../parlay/utilities.h"
template <class Graph>
struct EdgeBitmap {
    Graph& G;
    parlay::sequence<uint64_t> bitmap;
    parlay::sequence<uint64_t> offset;

    EdgeBitmap(Graph& G_) : G(G_) {
        size_t n=G.n;
        auto words = parlay::sequence<uint64_t>::from_function(n,[&](size_t i){
            return (G.offsets[i+1]-G.offsets[i]+63)>>6;
        });
        auto ps = parlay::scan(words); 
        offset = std::move(ps.first);
        offset.push_back(ps.second);
        bitmap = parlay::sequence<uint64_t>(offset[n], UINT64_MAX);
        parlay::parallel_for(0,n,[&](size_t u){
            uint64_t deg = G.offsets[u+1]-G.offsets[u];
            if(!(deg & 63)) return;
            uint64_t w = (deg + 63) >> 6;
            uint64_t last = offset[u] + w - 1;
            bitmap[last] = (1ull << (deg & 63)) - 1;
        });
    }

    inline void mark_dead(size_t s, uint64_t d_off) {
        __atomic_fetch_and(&bitmap[offset[s] + (d_off >> 6)], ~(1ull << (d_off & 63)), __ATOMIC_RELAXED);
    }
    inline void seq_mark_dead(size_t s, uint64_t d_off) {
        bitmap[offset[s] + (d_off >> 6)] &= ~(1ull << (d_off & 63));
    }

    template <class Cond, class Update>
    inline void visit(size_t s, Cond&& cond, Update&& update) {
        size_t WL = offset[s];
        size_t WR = offset[s+1];
        size_t base = G.offsets[s];
        if (WR - WL < 8) {
            for(size_t w = WL; w < WR; ++w){
                uint64_t m = bitmap[w];
                while(m){
                    uint64_t b = __builtin_ctzll(m);
                    size_t j = ((w - WL) << 6) + b;
                    size_t d = G.edges[base + j].v;
                    if (cond(d)) { update(d); }
                    else { seq_mark_dead(s, j); }
                    m &= m - 1;
                }
            }
        }
        else {
            parlay::parallel_for(WL, WR, [&](size_t w){
                uint64_t m = bitmap[w];
                while(m){
                    uint64_t b = __builtin_ctzll(m);
                    size_t j = ((w - WL) << 6) + b;
                    size_t d = G.edges[base + j].v;
                    if (cond(d)) update(d);
                    else seq_mark_dead(s, j);
                    m &= m - 1;
                }
            }, 4);
        }
    }
    template <class Cond, class Update>
    inline uint32_t reduce_cnt(size_t s, Cond&& cond, Update&& update){
        size_t WL = offset[s];
        size_t WR = offset[s+1];
        size_t base = G.offsets[s];
        std::function<uint32_t(size_t,size_t)> rec = [&](size_t L,size_t R)->uint32_t{
            if(R-L < 4){
                uint32_t cnt = 0;
                for(size_t w=L; w<R; ++w){
                    uint64_t m = bitmap[w];
                    while(m){
                        uint64_t b = __builtin_ctzll(m);
                        size_t j = ((w-WL)<<6) + b;
                        size_t d = G.edges[base + j].v;
                        if(cond(d)) cnt+=update(d);
                        else seq_mark_dead(s,j);
                        m &= m-1;
                    }
                }
                return cnt;
            }
            size_t M = (L+R)>>1;
            uint32_t c0=0,c1=0;
            parlay::parallel_do(
                [&](){ c0 = rec(L,M); },
                [&](){ c1 = rec(M,R); }
            );
            return c0 + c1;
        };
        return rec(WL,WR);
    }
};