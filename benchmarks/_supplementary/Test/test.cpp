#include <parlay/io.h>
#include "hashbag.h"
#include <ParSet/ParSet.h>
#include "graph.h"
#include "verify.h"

struct times {
    double t1, t2, t3, t4;
};

template <class Graph>
inline times benchmark_parset(Graph& G, size_t s) {
    const size_t n = G.n;
    ParSet::Frontier frontier(n);
    parlay::sequence<uint64_t> seq(n, 0);
    parlay::sequence<bool> seq2(G.m, 0);
    auto perm = parlay::random_permutation<uint32_t>(n);
    parlay::internal::timer t; double t1=0, t2=0, t3=0, t4=0;
    for (size_t r = 0; r < (n + s - 1) / s; ++r) {
        t.start();
        parlay::parallel_for(r*s, std::min((r+1)*s,n), [&](size_t i) { frontier.insert_next(perm[i]); });
        t1 += t.stop();

        t.start();
        frontier.advance_to_next();
        t2 += t.stop();

        t.start();
        frontier.for_each([&](size_t i) { seq[i] += 1; });
        t3 += t.stop();

        parlay::parallel_for(r*s, std::min((r+1)*s,n), [&](size_t i) { frontier.insert_next(perm[i]); });
        frontier.advance_to_next();

        t.start();
        frontier.for_each([&](size_t s) { 
            parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t i) { 
                seq2[i] = 1; 
            }, 128);
        });
        t4 += t.stop();
    }
    return {t1, t2, t3, t4};
}

template <class Graph>
inline times benchmark_hashbag(Graph& G, size_t s) {
    const size_t n = G.n;
    hashbag<uint32_t> bag(n);
    parlay::sequence<uint32_t> frontier(s);
    parlay::sequence<uint64_t> seq(n, 0);
    parlay::sequence<bool> seq2(G.m, 0);
    auto perm = parlay::random_permutation<uint32_t>(n);
    double total = 0.0;
    parlay::internal::timer t; double t1=0, t2=0, t3=0, t4=0;
    for (size_t r = 0; r < (n + s - 1) / s; ++r) {
        t.start();
        parlay::parallel_for(r*s, std::min((r+1)*s,n), [&](size_t i) { bag.insert(perm[i]); });
        t1 += t.stop();

        t.start();
        size_t m = bag.pack_into(frontier);
        t2 += t.stop();

        t.start();
        parlay::parallel_for(0, m, [&](size_t i) { seq[frontier[i]] += 1; });
        t3 += t.stop();

        t.start();
        parlay::parallel_for(0, m, [&](size_t i) {
            size_t s = frontier[i];
            parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t j) { 
                seq2[j] = 1; 
            }, 128);
        }, 1);
        t4 += t.stop();
    }
    return {t1, t2, t3, t4};
}

template <class Graph>
inline times benchmark_pack(Graph& G, size_t s) {
    const size_t n = G.n;
    //hashbag<uint32_t> bag(n);
    ParSet::Frontier bag(n);
    parlay::sequence<uint32_t> frontier(s);
    parlay::sequence<uint64_t> seq(n, 0);
    parlay::sequence<bool> seq2(G.m, 0);
    auto perm = parlay::random_permutation<uint32_t>(n);
    double total = 0.0;
    parlay::internal::timer t; double t1=0, t2=0, t3=0, t4=0;
    for (size_t r = 0; r < (n + s - 1) / s; ++r) {
        t.start();
        parlay::parallel_for(r*s, std::min((r+1)*s,n), [&](size_t i) { bag.insert_next(perm[i]); });// bag.insert(perm[i]); });
        t1 += t.stop();

        t.start();
        bag.advance_to_next();
        size_t m = bag.pack_into(frontier);
        t2 += t.stop();
/*
        t.start();
        parlay::parallel_for(0, m, [&](size_t i) { seq[frontier[i]] += 1; });
        t3 += t.stop();

        t.start();
        parlay::parallel_for(0, m, [&](size_t i) {
            size_t s = frontier[i];
            parlay::parallel_for(G.offsets[s], G.offsets[s+1], [&](size_t j) { 
                seq2[j] = 1; 
            }, 128);
        }, 1);
        t4 += t.stop();*/
    }
    return {t1, t2, t3, t4};
}

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    Graph G(filepath);
    std::string graph_name = extract_graph_name(filepath);
    std::cout << "==================================================================\n";
    std::cout << "### Graph:  " << graph_name << "\n";
    std::cout << "### Threads: " << parlay::num_workers() << "\n";

    uint32_t n = G.n, s = std::atoi(argv[2]);
    std::cerr << "n = " << n << "\n";
    std::cerr << "s = " << s << "\n";
    /*
    auto a = benchmark_hashbag(G, s);
    auto b = benchmark_pack(G, s);
    std::cerr << "speedup t1 = " << a.t1 << "/" << b.t1 << " = " << a.t1/b.t1 << "\n";
    std::cerr << "speedup t2 = " << a.t2 << "/" << b.t2 << " = " << a.t2/b.t2 << "\n";
    std::cerr << "speedup t3 = " << a.t3 << "/" << b.t3 << " = " << a.t3/b.t3 << "\n";
    std::cerr << "speedup t4 = " << a.t4 << "/" << b.t4 << " = " << a.t4/b.t4 << "\n";
    std::cerr << "speedup t123 = " << (a.t1+a.t2+a.t3)/(b.t1+b.t2+b.t3) << "\n";
    std::cerr << "speedup t124 = " << (a.t1+a.t2+a.t4)/(b.t1+b.t2+b.t4 ) << "\n";*/
    auto b = benchmark_pack(G, s);
    std::cerr << b.t1 << "   "<< b.t2 << "\n";
    return 0;
}