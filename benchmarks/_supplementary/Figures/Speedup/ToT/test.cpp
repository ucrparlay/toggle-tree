#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <parlay/io.h>
#include <parlay/parallel.h>
#include <graph_io/graph_io.h>

#include "../../../../BFS/ToT/BFS.h"
#include "../../../../Coloring/ToT/Coloring.h"
#include "../../../../KCore/ToT/KCore.h"

namespace {

std::vector<size_t> worker_counts() {
    return {1, 2, 4, 12, 24, 48, 96, 192};
}

double run_bfs_rounds(graph_io::Graph<>& G, const parlay::sequence<uint32_t>& perm, uint32_t num_rounds) {
    parlay::internal::timer t;
    double tt = 0.0;
    double total = 0.0;
    parlay::sequence<uint32_t> result;
    uint32_t base = 0;

    for (uint32_t i = 0; i < num_rounds + base; i++) {
        auto s = perm[i];
        t.start();
        result = BFS(G, s);
        tt = t.stop();
        if (!graph_io::availability(result, 0.1)) {
            base++;
            continue;
        }
        t.start();
        result = BFS(G, s);
        tt = t.stop();
        total += tt;
    }

    return total / num_rounds;
}

template <class Algorithm>
double run_rounds(graph_io::Graph<>& G, uint32_t num_rounds, Algorithm&& algorithm) {
    parlay::internal::timer t;
    t.start();
    auto result = algorithm(G);
    t.stop();

    double total = 0.0;
    for (uint32_t round = 0; round < num_rounds; round++) {
        t.start();
        result = algorithm(G);
        double tt = t.stop();
        total += tt;
    }
    return total / num_rounds;
}

template <class F>
void run_speedup(const std::string& graph_name, const std::string& name,
                 const std::string& outpath, const std::vector<size_t>& counts,
                 F&& run_for_threads) {
    std::cout << "==================================================================\n";
    std::cout << std::right << std::setw(66) << ("Algorithm: " + name) << "\n";
    std::vector<double> averages;
    averages.reserve(counts.size());
    for (size_t p : counts) {
        double avg = 0.0;
        parlay::execute_with_scheduler(static_cast<unsigned int>(p), [&] {
            avg = run_for_threads();
        });
        std::cout << "threads=" << p << " avg=" << avg << " sec\n";
        averages.push_back(avg);
    }

    if (averages.empty()) return;

    std::ofstream out(outpath, std::ios::app);
    if (!out) {
        std::cerr << "Error: cannot open output file " << outpath << "\n";
        std::exit(1);
    }

    const double baseline = averages.front();
    out << std::setprecision(12);
    out << graph_name;
    for (double avg : averages) {
        out << ' ' << (baseline / avg);
    }
    out << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <graph.{bin|adj}> [rounds]\n";
        return 1;
    }

    const char* filepath = argv[1];
    uint32_t num_rounds = (argc == 2) ? 3 : std::atoi(argv[2]);

    graph_io::Graph G(filepath);

    if (G.n < num_rounds) {
        std::cerr << "Error: graph has fewer vertices than requested rounds.\n";
        return 1;
    }

    auto perm = parlay::random_permutation<uint32_t>(G.n);
    auto counts = worker_counts();

    std::cout << "==================================================================\n";
    std::cout << std::right << std::setw(66) << ("Graph: " + G.name) << "\n";
    std::cout << "Load: " << G.load_time << "\n";
    std::cout << "Threads: 1 2 4 12 24 48 96 192  Rounds: " << num_rounds << "\n";
    std::cout << "Outputs: bfs.txt kcore.txt coloring.txt\n";

    run_speedup(G.name, "BFS", "bfs.txt", counts, [&] {
        return run_bfs_rounds(G, perm, num_rounds);
    });

    run_speedup(G.name, "KCore", "kcore.txt", counts, [&] {
        return run_rounds(G, num_rounds, [](auto& graph) { return KCore(graph); });
    });

    run_speedup(G.name, "Coloring", "coloring.txt", counts, [&] {
        return run_rounds(G, num_rounds, [](auto& graph) { return Coloring(graph); });
    });

    return 0;
}
