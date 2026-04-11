#include "BFS.h"
#include "verify.h"

namespace gbbs {

template <class Graph>
double BFS_runner(Graph& G, commandLine P) {
    std::cout << "==================================================================" << std::endl;
    const char* dumppath = P.getOptionValue("-dump") == nullptr ? "disabled" : P.getOptionValue("-dump");
    int num_rounds = std::atoi(P.getOptionValue("-num_rounds"));
    std::cout << std::right << std::setw(66) << ("Graph: " + graph_io::extract_graph_name(P.getArgument(0))) << "\n";
    std::cout << "Dump: " << dumppath << "\n";
    std::cout << "Threads: " << num_workers() << "  Rounds: " << num_rounds << "\n";

    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    parlay::sequence<uint32_t> result; uint32_t base = 0;
    for (uint32_t i = 0; i < num_rounds + base; i++) {
        auto s = perm[i];
        t.start(); result = BFS(G, s); tt = t.stop();
        if (!graph_io::availability(result, 0.1)) { base++; continue; }
        std::cout << "    Round " << i + 1 - base << "  source: " << s << "  Warmup: "  << std::setprecision(2) << tt << std::setprecision(6);
        t.start(); result = BFS(G, s); tt = t.stop();
        std::cout << "  time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;

    graph_io::process_result(dumppath, P.getArgument(0), ttt, result, true, "../../benchmarks/BFS");
    std::exit(0);
    return ttt;
}

}  // namespace gbbs

generate_main(gbbs::BFS_runner, false);
