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
    std::cout << "### Threads: " << num_workers();
    const char* dumppath = P.getOptionValue("-dump") == nullptr ? "disabled" : P.getOptionValue("-dump");
    std::cout << "  Dump: " << dumppath << "\n";
    
    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    t.start();
    parlay::sequence<uint32_t> result;
    for (int i = 0; i < 3; i++) {
        auto s = perm[i];
        std::cout << "Round " << i + 1 << "  source = " << s;
        t.start();
        result = BFS(G, s);
        std::cout << "  Warmup = " << t.stop();
        t.start();
        result = BFS(G, s);
        tt = t.stop();
        std::cout << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= 3;

    process_result(dumppath, P.getArgument(0), ttt, result, true, "../../benchmarks/BFS");
    std::exit(0);
    return tt;
}

}  // namespace gbbs

generate_main(gbbs::BFS_runner, false);
