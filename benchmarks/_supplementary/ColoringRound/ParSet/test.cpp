#include <parlay/io.h>
#include "verify.h"
#include "graph.h"
#include "Coloring.h"
#define Algorithm Coloring

int main(int argc, char** argv) {
    Graph G;
    const char* filepath = argv[1];
    G.read_graph(filepath);
    std::string graph_name = extract_graph_name(filepath);
    std::cout << "==================================================================\n";
    std::cout << "Graph:  " << graph_name << "\n";

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

    process_result("", filepath, ttt, result, true);  
    return 0;
}