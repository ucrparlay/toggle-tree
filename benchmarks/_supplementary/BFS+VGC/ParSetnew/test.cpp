#include <parlay/io.h>
#include <GraphIO/GraphIO.h>
#include "BFS.h"
#define Algorithm BFS

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    uint32_t num_rounds = (argc == 2) ? 3 : std::atoi(argv[2]);
    const char* dumppath = (argc == 3) ? "disabled" : argv[3];

    GraphIO::Graph G(filepath);
    std::cout << "==================================================================\n";
    std::cout << std::right << std::setw(66) << ("Graph: " + G.name) << "\n";
    std::cout << "Load: " << G.load_time << "  Dump: " << dumppath << "\n";
    std::cout << "Threads: " << parlay::num_workers() << "  Rounds: " << num_rounds << "\n";
    
    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    parlay::sequence<uint32_t> result; uint32_t base = 0;
    for (uint32_t i = 0; i < num_rounds + base; i++) {
        auto s = perm[i];
        t.start(); result = Algorithm(G, s); tt = t.stop();
        if (!GraphIO::availability(result, 0.1)) { base++; continue; }
        std::cout << "    Round " << i + 1 - base << "  source: " << s << "  Warmup: "  << std::setprecision(2) << tt << std::setprecision(6);
        t.start(); result = Algorithm(G, s); tt = t.stop();
        std::cout << "  time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;
    
    GraphIO::process_result(dumppath, filepath, ttt, result, true);  
    return 0;
}