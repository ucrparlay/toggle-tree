#pragma once
#include <ParSet/ParSet.h>

static inline int32_t write_min_old(int32_t& ref,int32_t v){
    int32_t old=ref;
    while(old>v&&!__atomic_compare_exchange_n(&ref,&old,v,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED));
    return old;
}

struct Tree {
    size_t leaf_layer;
    parlay::sequence<parlay::sequence<int32_t>> node;
    Tree(size_t n){
        size_t m=1,h=1;while(m<n)m<<=1,++h;
        node=parlay::sequence<parlay::sequence<int32_t>>(h);
        for(size_t i=0;i<h;i++)node[i]=parlay::sequence<int32_t>(size_t(1)<<i,INT32_MAX);
        leaf_layer = node.size()-1;
    }
    template <class Dist>
    void repair(ParSet::Frontier& frontier, Dist& dist){
        frontier.for_each([&](size_t s){
            int32_t v=dist[s];
            if(v==INT32_MAX)return;
            node[leaf_layer][s]=v;
            for(size_t h=leaf_layer,i=s;h;--h,i>>=1){
                int32_t old=write_min_old(node[h-1][i>>1],v);
                if(old<v)break;
            }
        });
    }
    int32_t get_min(){ return node[0][0]; }

    int32_t extract_min_rec(size_t layer, size_t index, int32_t threshold, ParSet::Frontier& frontier){ 
        int32_t &x = node[layer][index];
        if (layer == leaf_layer) {
            frontier.insert_next(index);
            return x = INT32_MAX;
        }
        size_t lc = index<<1, rc = lc|1;
        
        if (node[layer+1][lc] > threshold) return x = std::min(node[layer+1][lc], extract_min_rec(layer+1,rc,threshold,frontier));
        if (node[layer+1][rc] > threshold) return x = std::min(extract_min_rec(layer+1,lc,threshold,frontier), node[layer+1][rc]);

        constexpr size_t PAR_CUTOFF=25;
        int32_t L,R;
        if (leaf_layer-layer < PAR_CUTOFF) {
            L=extract_min_rec(layer+1,lc,threshold,frontier);
            R=extract_min_rec(layer+1,rc,threshold,frontier);
        }
        else{
            parlay::parallel_do(
                [&]{L = extract_min_rec(layer+1,lc,threshold,frontier);},
                [&]{R = extract_min_rec(layer+1,rc,threshold,frontier);}
            );
        }
        return x = std::min(L, R);
    }
    void extract_min(ParSet::Frontier& frontier){
        if (get_min() == INT32_MAX) return;
        extract_min_rec(0, 0, get_min(), frontier);
    }
};

static inline bool write_min(int32_t& ref, int32_t v){
    int32_t old = ref;
    while (old > v && !__atomic_compare_exchange_n(&ref, &old, v, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    return old > v;
}

template <class Graph>
parlay::sequence<int32_t> Dijkstra(Graph& G, size_t source=0) {
    const size_t n = G.n;
    auto frontier = ParSet::Frontier(n);
    auto tree = Tree(n);
    auto dist = parlay::sequence<int32_t>(n, INT32_MAX); dist[source] = 0;
    frontier.insert_next(source);
    parlay::internal::timer t; double t1=0,t2=0,t3=0;
    for (uint32_t round = 0;  ;round++) {
        t.start();
        frontier.advance_to_next();
        tree.repair(frontier, dist);
        //std::cerr << "get_min(): " << tree.get_min() << "\n";
        t1 += t.stop();
        t.start();
        tree.extract_min(frontier);
        t2 += t.stop();
        t.start();
        if(!frontier.advance_to_next()) break;
        frontier.for_each([&](uint32_t s) { 
            int32_t dist_s = dist[s];
            ParSet::adaptive_for(G.offsets[s], G.offsets[s + 1], [&](size_t i) {
                uint32_t d = G.edges[i].v;
                int32_t w = G.edges[i].w;
                if (dist[d] > dist_s + w && write_min(dist[d], dist_s + w)) {
                    frontier.insert_next(d);
                }
            });
        });
        t3 += t.stop();
    }
    std::cerr << "\nt1 = " << t1 << "\n";
    std::cerr << "t2 = " << t2 << "\n";
    std::cerr << "t3 = " << t3 << "\n";
    return dist;
}