#include <parlay/io.h>
#include "verify.h"
#include "graph.h"
#include "BellmanFord.h"
#define Algorithm BellmanFord

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    Graph<int32_t> G(filepath);
    const char* dumppath = (argc == 2) ? "disabled" : argv[2];
    std::cout << "==================================================================\n";
    std::cout << "### Graph:  " << G.name << "\n";
    std::cout << "### Threads: " << parlay::num_workers() << "  Dump: " << dumppath << "\n";

    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    t.start();
    parlay::sequence<int32_t> result;
    for (int i = 0; i < 3; i++) {
        auto s = perm[i];
        std::cout << "Round " << i + 1 << "  source = " << s;
        t.start();
        result = Algorithm(G, s);
        std::cout << "  Warmup = " << t.stop();
        t.start();
        result = Algorithm(G, s);
        tt = t.stop();
        std::cout << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= 3;

    process_result(dumppath, filepath, ttt, result, true);  
    return 0;
}