// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include "../../parallel/parallel.h"
#include "../../parallel/atomic.h"
#include "../../data_structures/array.h"
#include "../../data_structures/index_set.h"
#include "../../data_structures/index_map.h"
#include "../../primitives/combine.h"
#include "../../primitives/reduce.h"

namespace toggle {

template <class Graph>
toggle::Array<typename Graph::weight_type> bellman_ford(Graph& G, uint64_t source = 0) {
    using T = typename Graph::weight_type;
    auto dist = toggle::Array<T>(G.n(), std::numeric_limits<T>::max());
    auto frontier = toggle::IndexSet(G.n(), false);
    auto next_frontier = toggle::IndexSet(G.n(), false);
    dist[source] = 0; frontier.insert(source);

    for (uint64_t round = 0; round < G.n() && !frontier.empty(); round++) {
        frontier.consume_each([&](uint64_t s) {
            T candidate = toggle::reduce<toggle::min, std::numeric_limits<T>::max()>(G.in_offset(s), G.in_offset(s+1), [&](uint64_t i) {
                auto [d, w] = G.weighted_in_edge(i);
                T dist_d = toggle::atomic_read(dist[d]);
                return dist_d > std::numeric_limits<T>::max() - w ? std::numeric_limits<T>::max() : dist_d + w;
            });
            toggle::write_min(dist[s], candidate);
            T dist_s = toggle::atomic_read(dist[s]);
            toggle::parallel_for(G.out_offset(s), G.out_offset(s+1), [&](uint64_t i) {
                auto [d, w] = G.weighted_out_edge(i);
                T dist_d = dist_s > std::numeric_limits<T>::max() - w ? std::numeric_limits<T>::max() : dist_s + w;
                if (toggle::write_min(dist[d], dist_d)) {
                    next_frontier.insert(d);
                }
            }, 256);
        });
        std::swap(frontier, next_frontier);
    }

    return dist;
}

template <class Graph>
toggle::Array<typename Graph::weight_type> dijkstra(Graph& G, size_t source = 0) {
    using T = typename Graph::weight_type;
    auto dist = toggle::Array<T>(G.n(), std::numeric_limits<T>::max());
    auto tree = toggle::IndexMap(dist);
    dist[source] = 0; tree.insert(source);

    while (true) {
        T min_value = tree.repair();
        if (min_value == std::numeric_limits<T>::max()) break;
        T threshold = min_value;
        tree.extract_min(threshold, [&] (size_t s) {
            T dist_s = toggle::atomic_read(dist[s]);
            toggle::parallel_for(G.out_offset(s), G.out_offset(s + 1), [&](size_t i) {
                auto [d, w] = G.weighted_out_edge(i);
                T dist_d = dist_s > std::numeric_limits<T>::max() - w ? std::numeric_limits<T>::max() : dist_s + w;
                if (toggle::write_min(dist[d], dist_d)) {
                    tree.insert(d);
                }
            }, 256);
        });
    }
    return dist;
}

}
