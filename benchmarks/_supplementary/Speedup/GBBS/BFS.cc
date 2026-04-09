#include "BFS.h"
#include "verify.h"

#include <filesystem>
#include <fstream>
#include <iomanip>

namespace gbbs {

static void append_result(const std::string& path, const std::string& graph_name,
                          const std::string& algorithm, size_t threads,
                          double avg_sec, int num_rounds) {
    bool need_header = !std::filesystem::exists(path);
    std::ofstream out(path, std::ios::app);
    if (need_header) {
        out << "graph\talgorithm\tthreads\tavg_sec\trounds\tload_sec\n";
    }
    out << graph_name << '\t' << algorithm << '\t' << threads << '\t'
        << avg_sec << '\t' << num_rounds << '\t' << 0.0 << '\n';
}

template <class Graph>
double BFS_runner(Graph& G, commandLine P) {
    std::cout << "==================================================================" << std::endl;
    int num_rounds = std::atoi(P.getOptionValue("-num_rounds"));
    const char* outpath = P.getOptionValue("-out");
    std::string graph_name = GraphIO::extract_graph_name(P.getArgument(0));
    std::cout << std::right << std::setw(66) << ("Graph: " + graph_name) << "\n";
    std::cout << "Threads: " << num_workers() << "  Rounds: " << num_rounds << "\n";

    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t;
    double tt = 0.0, total = 0.0;
    parlay::sequence<uint32_t> result;
    uint32_t base = 0;
    for (uint32_t i = 0; i < num_rounds + base; i++) {
        auto s = perm[i];
        t.start();
        result = BFS(G, s);
        tt = t.stop();
        if (!GraphIO::availability(result, 0.1)) {
            base++;
            continue;
        }
        t.start();
        result = BFS(G, s);
        tt = t.stop();
        total += tt;
    }
    double avg = total / num_rounds;
    std::cout << "algorithm=BFS threads=" << num_workers() << " avg=" << avg << " sec\n";
    if (outpath != nullptr) {
        append_result(outpath, graph_name, "BFS", num_workers(), avg, num_rounds);
    }
    std::exit(0);
    return avg;
}

}  // namespace gbbs

generate_main(gbbs::BFS_runner, false);
