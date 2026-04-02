#include <parlay/io.h>
#include <GraphIO/GraphIO.h>

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    GraphIO::Graph G(filepath);
    std::cerr << "Sorted: " << G.neighbors_sorted() << "\n";
    return 0;
}