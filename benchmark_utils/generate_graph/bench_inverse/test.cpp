#include <parlay/io.h>
#include <GraphIO/GraphIO.h>

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    GraphIO::Graph G(filepath);
    std::cerr << "Load: " << G.load_time << "\n";
    std::cerr << "G.m: " << G.m << "\n";
    return 0;
}