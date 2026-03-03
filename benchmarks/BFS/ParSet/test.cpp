#include <parlay/io.h>
#include "verify.h"
#include "graph.h"
#include "BFS.h"
#define Algorithm BFS

int main(int argc, char** argv) {
    Graph G;
    const char* filepath = argv[1];
    G.read_graph(filepath); if (G.n == 0) { std::cerr << "Failed in reading graph.\n"; return 0; }
    std::string graph_name = extract_graph_name(filepath);
    std::cout << "==================================================================\n";
    std::cout << "### Graph:  " << graph_name << "\n";
    std::cout << "### Threads: " << parlay::num_workers() << std::endl;

    parlay::internal::timer t; t.start();
    auto result = Algorithm(G,0); 
    result = Algorithm(G,100); 
    result = Algorithm(G,200); 
    result = Algorithm(G,300); 
    result = Algorithm(G,400);
    std::cout << "Warmup: " << 0.2*t.stop() << "\n";
    double ttt = 0;
    for (int round = 0; round < 3; round++) {
        for (int source = 0; source < 500; source +=100) {
            t.start();
            result = Algorithm(G,source);
            double tt = t.stop();
            std::cout << "Source " << source << "  Round " << round + 1 << " time = " << tt << " sec\n";
            ttt += tt;
        }
    }
 
    ttt /= 15;

    process_result(filepath, ttt, result, true);  
    return 0;
}

