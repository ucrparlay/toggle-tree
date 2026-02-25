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

#pragma once

#include "gbbs/gbbs.h"

namespace gbbs {

template <class W>
struct coloring_f {
    intE* p;
    coloring_f(intE* _p) : p(_p) {}
    inline bool cond(uintE d) { return (p[d] > 0); }
    inline bool updateAtomic(const uintE& s, const uintE& d, const W& wgh) {
        return (gbbs::fetch_and_add(&p[d], -1) == 1);
    }
    inline bool update(const uintE& s, const uintE& d, const W& w) { return updateAtomic(s,d,w); }
};

template <class Graph>
inline sequence<uintE> Coloring(Graph& G) {
    using W = typename Graph::weight_type;
    const size_t n = G.n;

    auto priorities = sequence<intE>(n);
    auto colors = sequence<uintE>::from_function(n, [](size_t i) { return UINT_E_MAX; });
    {
        auto P = parlay::random_permutation<uintE>(n);
        parallel_for(0, n, 1, [&](size_t i) {
            uintE our_deg = G.get_vertex(i).out_degree();
            uintE i_p = P[i];
            auto count_f = [&](uintE src, uintE ngh, const W& wgh) {
                uintE ngh_deg = G.get_vertex(ngh).out_degree();
                return (ngh_deg > our_deg) || ((ngh_deg == our_deg) && P[ngh] < i_p);
            };
            priorities[i] = G.get_vertex(i).out_neighbors().count(count_f);
        });
    }
    auto zero_map_f = [&](size_t i) { return priorities[i] == 0; };
    auto zero_map = parlay::delayed_seq<bool>(n, zero_map_f);
    auto roots = vertexSubset(n, parlay::pack_index<uintE>(zero_map));

    size_t finished = 0, rounds = 0;
    while (finished != n) {
        assert(roots.size() > 0);
        finished += roots.size();
        roots.toSparse();

        parallel_for(0, roots.size(), 1, [&](size_t i) {
            uintE v = roots.vtx(i);
            colors[v] = rounds;
        });

        auto new_roots = edgeMap(G, roots, coloring_f<W>(priorities.begin()), -1, sparse_blocked);
        roots = std::move(new_roots);
        rounds++;
    }
    return colors;
}
}  // namespace gbbs
