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
    const char* dumppath = (argc == 2) ? "disabled" : argv[2];
    std::cout << "==================================================================\n";
    std::cout << "### Graph:  " << graph_name << "\n";
    std::cout << "### Threads: " << parlay::num_workers() << "  Dump: " << dumppath << "\n";;
    
    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    std::cout << "Warmup: ";
    t.start();
    auto result = Algorithm(G, perm[0]);
    std::cout << t.stop() << " ";
    for (int i = 1; i < 5; i++) {
        auto s = perm[i];
        t.start();
        result = Algorithm(G, s);
        std::cout << t.stop() << " ";
    }
    std::cout << "\n";

    for (int i = 0; i < 5; i++) {
        auto s = perm[i];
        t.start();
        result = Algorithm(G, s);
        tt = t.stop();
        std::cout << "Round " << i + 1 << " source = " << s << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= 5;
    
    process_result(dumppath, filepath, ttt, result, true);  
    return 0;
}
