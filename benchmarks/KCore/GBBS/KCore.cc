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

#include "KCore.h"
#include "verify.h"

namespace gbbs {
template <class Graph>
double KCore_runner(Graph& G, commandLine P) {
    std::cout << "==================================================================" << std::endl;
    std::string gname = extract_graph_name(P.getArgument(0));
    std::cout << "### Graph: " << gname << std::endl;
    std::cout << "### Threads: " << num_workers() << std::endl;
    size_t num_buckets = 16;

    parlay::internal::timer t;
    t.start();
    auto result = KCore(G, num_buckets);
    double tt = t.stop(); 
    std::cout << "Warmup: " << tt << "\n"; 

    double ttt = 0;
    for (int round = 0; round < 3; round++) {
        t.start();
        result = KCore(G, num_buckets);
        tt = t.stop();
        std::cout << "Round " << round + 1 << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= 3;

    process_result(P.getArgument(0), ttt, result, true, "../../benchmarks/KCore");
    static std::ofstream null("/dev/null");
    std::cout.rdbuf(null.rdbuf());
    return ttt;
}
}  // namespace gbbs

generate_symmetric_main(gbbs::KCore_runner, false);
