#include <parlay/io.h>
#include "verify.h"
#include "graph.h"
#include "BFS.h"
#define Algorithm BFS

int main(int argc, char** argv) {
    Graph G;
    const char* filepath = argv[1];
    G.read_graph(filepath);
    std::string graph_name = extract_graph_name(filepath);
    std::cout << "==================================================================\n";
    std::cout << "### Graph:  " << graph_name << "\n";

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
            parlay::internal::timer t;
            t.start();
            result = Algorithm(G,source);
            double tt = t.stop();
            std::cout << "Source " << source << "  Round " << round + 1 << " time = " << tt << " sec\n";
            ttt += 0.2*tt;
        }
    }
 
    ttt /= 3;

    process_result(filepath, ttt, result, true);  
    return 0;
}