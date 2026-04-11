#include "GraphColoring.h"
#include "verify.h"

namespace gbbs {

template <class Graph>
double Coloring_runner(Graph& G, commandLine P) {
    std::cout << "==================================================================" << std::endl;
    const char* dumppath = P.getOptionValue("-dump") == nullptr ? "disabled" : P.getOptionValue("-dump");
    int num_rounds = std::atoi(P.getOptionValue("-num_rounds"));
    std::cout << std::right << std::setw(66) << ("Graph: " + graph_io::extract_graph_name(P.getArgument(0))) << "\n";
    std::cout << "Dump: " << dumppath << "\n";
    std::cout << "Threads: " << num_workers() << "  Rounds: " << num_rounds << "\n";

    parlay::internal::timer t; double ttt = 0;
    t.start();
    auto result = Coloring(G, true);
    std::cout << "    Warmup: " << t.stop() << "\n"; 
    for (int round = 0; round < num_rounds; round++) {
        t.start();
        result = Coloring(G, true);
        double tt = t.stop();
        std::cout << "    Round " << round + 1 << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;

    graph_io::process_result(dumppath, P.getArgument(0), ttt, result, true, "../../benchmarks/Coloring");
    std::exit(0);
    return ttt;
}

}  // namespace gbbs
generate_main(gbbs::Coloring_runner, false);
