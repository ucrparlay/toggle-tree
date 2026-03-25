#include <parlay/io.h>
#include "graph.h"
#include "verify.h"

int main(int argc, char** argv) {
    const char* filepath = argv[1];
    const char* storepath = argv[2];
    const char* range = argv[3];
    Graph<int32_t> G(filepath, std::atoi(range));
    std::string name = storepath + G.name;
    if (name.find("_sym") != std::string::npos) { name = name.substr(0, name.find("_sym")); }
    if (name.find("_wgh") != std::string::npos) { name = name.substr(0, name.find("_wgh")); }
    if (G.symmetrized) name += "_sym";
    if (G.weighted) name += "_wgh";
    if (std::string(range) != "0") { name += range; }
    else { name += "log"; }
    name += ".bin";
    std::cerr << "New graph name = " << name << "\n";
    G.write_bin_format(name.c_str());
    return 0;
}