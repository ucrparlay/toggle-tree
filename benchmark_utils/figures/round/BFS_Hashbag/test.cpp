#include <parlay/io.h>
#include <graph_io/graph_io.h>
#include "BFS.h"
#include <fstream>
#include <sys/stat.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <graph.bin> <output.csv>\n";
        return 1;
    }
    const char* filepath = argv[1];
    const char* csvpath = argv[2];

    graph_io::Graph G(filepath);
    std::cout << "==================================================================\n";
    std::cout << std::right << std::setw(66) << ("Graph: " + G.name) << "\n";
    std::cout << "Threads: " << parlay::num_workers() << "\n";

    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::sequence<uint32_t> result;
    std::vector<std::pair<size_t, double>> rounds;
    uint32_t source = 0;
    bool found = false;
    for (size_t i = 0; i < perm.size(); ++i) {
        source = perm[i];
        std::tie(result, rounds) = BFS(G, source);
        if (graph_io::availability(result, 0.1)) {
            found = true;
            break;
        }
    }
    if (!found) {
        std::cerr << "Failed to find a BFS source with sufficient coverage.\n";
        return 1;
    }

    std::cout << "Source: " << source << "\n";
    std::cout << "Frontier rounds recorded: " << rounds.size() << "\n";

    struct stat st;
    bool need_header = (stat(csvpath, &st) != 0);
    std::ofstream csv(csvpath, std::ios::app);
    if (need_header) csv << "frontier_size,time\n";
    for (auto [frontier_size, tt] : rounds) {
        csv << frontier_size << "," << tt << "\n";
    }

    return 0;
}
