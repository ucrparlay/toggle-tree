#pragma once

struct Active{
    Bitmap active;
    parlay::sequence<uint32_t> pack;
    uint32_t pack_round,round;
    Active(size_t n,uint64_t _pack_round=20):active(n,true),pack_round(_pack_round),round(0){pack_once();}
    inline bool exist_active(){return pack.size();}
    inline void set_fork_depth(int depth) { return; }
    inline bool is_active(size_t i){return active.is_true(i);}
    inline void mark_dead(size_t i){active.mark_false(i);}
    inline void pack_once(){
        size_t n=active.n;
        auto ids=parlay::delayed_seq<uint32_t>(n,[&](size_t i){return (uint32_t)i;});
        pack=parlay::filter(ids,[&](uint32_t i){return active.is_true(i);});
    }
    template<class F> inline void parallel_do(F&& f){
        if(++round==pack_round){pack_once();round=0;}
        parlay::parallel_for(0,pack.size(),[&](size_t i){f(pack[i]);});
    }
    template<class Graph,class Source,class Cond,class Update>
    inline void EdgeMapSparse(Graph& G,Source&& source,Cond&& cond,Update&& update){
        parallel_do([&](uint32_t s){
            source(s);
            size_t st=G.offsets[s],ed=G.offsets[s+1];
            adaptive_for(st,ed,[&](size_t j){
                size_t d=G.edges[j].v;
                if(cond(d)) update(d);
            });
        });
    }
};
/*
struct Active{
    Bitmap active;
    parlay::sequence<uint32_t> pack;
    Active(size_t n):active(n,true){pack_once();}
    inline bool exist_active(){return pack.size();}
    inline bool is_active(size_t i){return active.is_true(i);}
    inline void mark_dead(size_t i){active.mark_false(i);}
    inline void pack_once(){
        size_t n=active.n;
        auto ids=parlay::delayed_seq<uint32_t>(n,[&](size_t i){return (uint32_t)i;});
        pack=parlay::filter(ids,[&](uint32_t i){return active.is_true(i);});
    }
    inline void maybe_repack(){
        if(pack.size()<=256) return;
        uint32_t dead=0;
        for(size_t i=0;i<256;i++) dead+=!active.is_true(pack[i]);
        if(dead>=128) pack_once();
    }
    template<class F> inline void parallel_do(F&& f){
        maybe_repack();
        parlay::parallel_for(0,pack.size(),[&](size_t i){f(pack[i]);});
    }
    template<class Graph,class Source,class Cond,class Update>
    inline void EdgeMapSparse(Graph& G,Source&& source,Cond&& cond,Update&& update){
        parallel_do([&](uint32_t s){
            source(s);
            size_t st=G.offsets[s],ed=G.offsets[s+1];
            adaptive_for(st,ed,[&](size_t j){
                size_t d=G.edges[j].v;
                if(cond(d)) update(d);
            });
        });
    }
};
*/