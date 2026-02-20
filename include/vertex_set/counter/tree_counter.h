#pragma once
#include <cstdint>
#include "parlay/parallel.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"

inline uint32_t hi32(uint64_t x){return x>>32;}
inline uint32_t lo32(uint64_t x){return (uint32_t)x;}
inline uint64_t set_hi(uint64_t x,uint32_t v){return (x&0xFFFFFFFFull)|(uint64_t(v)<<32);}
inline uint64_t set_lo(uint64_t x,uint32_t v){return (x&0xFFFFFFFF00000000ull)|uint64_t(v);}
inline uint32_t vmax(uint64_t x){uint32_t a=hi32(x),b=lo32(x);return a>b?a:b;}

struct MaxTree{
    parlay::sequence<uint64_t> tree;
    size_t n,L,H;
    size_t fork_depth;
    inline size_t base(size_t h){return h?(1ull<<(h-1)):0;}

    template<class Graph>
    MaxTree(Graph& G){
        n=G.n;L=1;while(L<n)L<<=1;H=0;while((1ull<<H)<L)H++;
        fork_depth = (H > 12 ? H - 12 : 0);
        tree=parlay::sequence<uint64_t>(1+L,0);

        size_t lb=H?(1ull<<(H-1)):0;
        parlay::parallel_for(0,L>>1,[&](size_t i){
            uint32_t a=0,b=0;
            size_t x=i<<1,y=x+1;
            if(x<n)a=G.offsets[x+1]-G.offsets[x];
            if(y<n)b=G.offsets[y+1]-G.offsets[y];
            tree[lb+i]=(uint64_t(a)<<32)|b;
        });

        for(ssize_t h=H-1;h>=1;--h){
            size_t c=1ull<<(h-1),nxt=1ull<<h;
            adaptive_for(0,c,[&](size_t i){
                uint64_t Lc=tree[nxt+(i<<1)],Rc=tree[nxt+(i<<1)+1];
                tree[c+i]=(uint64_t(vmax(Lc))<<32)|vmax(Rc);
            });
        }

        uint32_t r = H ? vmax(tree[1]) : (n?G.offsets[1]-G.offsets[0]:0);
        tree[0]=uint64_t(r)<<32; 
    }

    inline void bubble_up(size_t h,size_t idx){
        for(;;){
            uint64_t cur=__atomic_load_n(&tree[base(h)+(idx>>1)],__ATOMIC_RELAXED);
            uint32_t m=vmax(cur);
            if(!h)return;
            size_t p=idx>>1,w=base(h-1)+(p>>1);
            uint64_t old=__atomic_load_n(&tree[w],__ATOMIC_RELAXED);
            for(;;){
                uint32_t pv=(p&1)?lo32(old):hi32(old);
                if(m>=pv)return;
                uint64_t neu=(p&1)?set_lo(old,m):set_hi(old,m);
                if(__atomic_compare_exchange_n(&tree[w],&old,neu,false,__ATOMIC_RELAXED,__ATOMIC_RELAXED))break;
            }
            h--;idx=p;
        }
    }

    inline bool decrement(size_t i){
        size_t w=base(H)+(i>>1);
        bool z;
        if(i&1){
            z = __atomic_fetch_sub(&tree[w],1ull,__ATOMIC_RELAXED) == 1;
        }else{
            z = __atomic_fetch_sub(&tree[w],1ull<<32,__ATOMIC_RELAXED)==1;
        }
        bubble_up(H,i);
        return z;
    }

    inline void set_zero(size_t i){
        size_t w=base(H)+(i>>1);
        __atomic_store_n(&tree[w],(i&1)?set_lo(tree[w],0):set_hi(tree[w],0),__ATOMIC_RELAXED);
        bubble_up(H,i);
    }

    template<class F>
    inline void visit_layer(size_t h,size_t idx,uint32_t m,F& f){
        if(h==H){ f(idx); return; }
        uint64_t w = tree[base(h+1)+idx];
        if (lo32(w)!=m){ visit_layer(h+1,idx<<1,m,f); return; }
        if (hi32(w)!=m){ visit_layer(h+1,(idx<<1)|1,m,f); return; }
        if (h > fork_depth) {
            visit_layer(h+1,idx<<1,m,f);
            visit_layer(h+1,(idx<<1)|1,m,f);
            return;
        }
        parlay::parallel_do(
            [&](){visit_layer(h+1,idx<<1,m,f);},
            [&](){visit_layer(h+1,(idx<<1)|1,m,f);}
        );
    }

    template<class F>
    inline void parallel_do(F&& f){
        visit_layer(0,0,hi32(tree[0]),f);
    }

    inline uint32_t load(size_t i) {
        uint64_t w = tree[base(H) + (i >> 1)];
        return (i & 1) ? lo32(w) : hi32(w);
    }
};