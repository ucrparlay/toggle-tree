// This code is part of the project "Theoretically Efficient Parallel Graph
// Algorithms Can Be Fast and Scalable", presented at Symposium on Parallelism
// in Algorithms and Architectures, 2018.
// Copyright (c) 2018 Laxman Dhulipala, Guy Blelloch, and Julian Shun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all  copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "BFS.h"
#include "verify.h"

namespace gbbs {

template <class Graph>
double BFS_runner(Graph& G, commandLine P) {
    std::cout << "==================================================================" << std::endl;
    std::string gname = extract_graph_name(P.getArgument(0));
    std::cout << "### Graph: " << gname << std::endl;
    std::cout << "### Threads: " << num_workers() << std::endl;

    parlay::internal::timer t;
    t.start();
    auto parents = BFS(G, 0);
    parents = BFS(G, 100);
    parents = BFS(G, 200);
    parents = BFS(G, 300);
    parents = BFS(G, 400);
    double tt = t.stop();
    std::cout << "Warmup: " << 0.2*tt << "\n";


    double ttt = 0;
    
    for (int round = 0; round < 3; round++) {
        for (int source = 0; source < 500; source += 100) {
            t.start();
            parents = BFS(G, source);
            tt = t.stop();
            std::cout << "Source " << source << "  Round " << round + 1 << " time = " << tt << " sec\n";
            ttt += tt;
        }
    }
    
    ttt /= 15;

    process_result(P.getArgument(0), ttt, parents, true, "../../benchmarks/BFS");
    static std::ofstream null("/dev/null");
    std::cout.rdbuf(null.rdbuf());
    return tt;
}

}  // namespace gbbs

generate_main(gbbs::BFS_runner, false);
