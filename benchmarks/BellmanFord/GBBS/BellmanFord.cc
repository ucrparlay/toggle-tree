#include "BellmanFord.h"
#include "verify.h"

namespace gbbs {
template <class Graph>
double BellmanFord_runner(Graph& G, commandLine P) {
    std::cout << "==================================================================" << std::endl;
    const char* dumppath = P.getOptionValue("-dump") == nullptr ? "disabled" : P.getOptionValue("-dump");
    int num_rounds = std::atoi(P.getOptionValue("-num_rounds"));
    std::cout << std::right << std::setw(66) << ("Graph: " + GraphIO::extract_graph_name(P.getArgument(0))) << "\n";
    std::cout << "Dump: " << dumppath << "\n";
    std::cout << "Threads: " << num_workers() << "  Rounds: " << num_rounds << "\n";

    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    t.start();
    parlay::sequence<int32_t> result;
    for (int i = 0; i < num_rounds; i++) {
        auto s = perm[num_rounds - i - 1];
        std::cout << "    Round " << i + 1 << "  source: " << s;
        t.start();
        result = BellmanFord(G, s);
        std::cout<< "  Warmup: "  << std::setprecision(2) << t.stop() << std::setprecision(6);
        t.start();
        result = BellmanFord(G, s);
        tt = t.stop();
        std::cout << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;

    GraphIO::process_result(dumppath, P.getArgument(0), ttt, result, true, "../../benchmarks/BellmanFord");
    std::exit(0);
    return ttt;
}
}  // namespace gbbs

generate_weighted_main(gbbs::BellmanFord_runner, false);
