#include <parlay/io.h>
#include "verify.h"
#include "graph.h"
#include "Coloring.h"
#define Algorithm Coloring

int main(int argc, char** argv) {
    Graph G;
    const char* filepath = argv[1];
    G.read_graph(filepath); if (G.n == 0) { std::cerr << "Failed in reading graph.\n"; return 0; }
    std::string graph_name = extract_graph_name(filepath);
    const char* dumppath = (argc == 2) ? "disabled" : argv[2];
    std::cout << "==================================================================\n";
    std::cout << "### Graph:  " << graph_name << "\n";
    std::cout << "### Threads: " << parlay::num_workers() << "  Dump: " << dumppath << "\n";

    parlay::internal::timer t; t.start();
    auto result = Algorithm(G);
    std::cout << "Warmup: " << t.stop() << "\n";

    double ttt = 0;
    for (int round = 0; round < 3; round++) {
        t.start();
        result = Algorithm(G);
        double tt = t.stop();
        std::cout << "Round " << round + 1 << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= 3;

    process_result(dumppath, filepath, ttt, result, true);  
    return 0;
}