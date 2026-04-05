#include <parlay/io.h>
#include <GraphIO/GraphIO.h>
#include "KCore.h"
#include <fstream>
#include <sys/stat.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <graph.bin> <output.csv>\n";
        return 1;
    }
    const char* filepath = argv[1];
    const char* csvpath  = argv[2];

    GraphIO::Graph G(filepath);
    std::cout << "==================================================================\n";
    std::cout << std::right << std::setw(66) << ("Graph: " + G.name) << "\n";
    std::cout << "Threads: " << parlay::num_workers() << "\n";

    auto [result, rounds] = KCore(G);
    std::cout << "Frontier rounds recorded: " << rounds.size() << "\n";

    // Append to CSV; write header only if file does not yet exist
    struct stat st;
    bool need_header = (stat(csvpath, &st) != 0);
    std::ofstream csv(csvpath, std::ios::app);
    if (need_header) csv << "frontier_size,time\n";
    for (auto& [sz, tt] : rounds) {
        csv << sz << "," << tt << "\n";
    }

    return 0;
}
