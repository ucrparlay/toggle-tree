#include <parlay/io.h>
#include <graph_io/graph_io.h>
#include "Coloring.h"
#define Algorithm Coloring

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    uint32_t num_rounds = (argc == 2) ? 3 : std::atoi(argv[2]);
    const char* dumppath = (argc == 3) ? "disabled" : argv[3];

    graph_io::Graph G(filepath);
    std::cout << "==================================================================\n";
    std::cout << std::right << std::setw(66) << ("Graph: " + G.name) << "\n";
    std::cout << "Load: " << G.load_time << "  Dump: " << dumppath << "\n";
    std::cout << "Threads: " << parlay::num_workers() << "  Rounds: " << num_rounds << "\n";

    parlay::internal::timer t; t.start();
    auto result = Algorithm(G);
    std::cout << "    Warmup: " << t.stop() << "\n";
    double ttt = 0;
    for (int round = 0; round < num_rounds; round++) {
        t.start();
        result = Algorithm(G);
        double tt = t.stop();
        std::cout << "    Round " << round + 1 << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;

    graph_io::process_result(ttt, result, "..", G.name, "Hashbag", dumppath); 
    return 0;
}