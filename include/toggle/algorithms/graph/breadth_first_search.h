// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <utility>
#include "../../parallel/parallel.h"
#include "../../parallel/atomic.h"
#include "../../parallel/word.h"
#include "../../data_structures/array.h"
#include "../../data_structures/index_set.h"
#include "../../data_structures/local_queue.h"
#include "../../primitives/combine.h"
#include "../../primitives/reduce.h"

namespace toggle {

template <class Graph>
toggle::Array<uint32_t> breadth_first_search(Graph& G, uint64_t source = 0) {
    auto active = toggle::IndexSet(G.n(), true);
    auto frontier = toggle::IndexSet(G.n(), false);
    auto next_frontier = toggle::IndexSet(G.n(), false);
    auto reduce_edge = [&]() { return frontier.reduce<toggle::sum, uint64_t(0)>([&](uint64_t s) { return G.out_offset(s + 1) - G.out_offset(s); }); };
    auto dist = toggle::Array<uint32_t>(G.n(), UINT32_MAX);
    next_frontier.insert(source);
    active.remove(source);
    uint64_t mode = 0, stride = 1;

    for (uint64_t round = 0; ; round++) {
        if (mode == 0) {
            std::swap(frontier, next_frontier);
            if (frontier.empty()) break;
            if (reduce_edge() > (G.m() >> 3)) {
                frontier.consume_each([&](uint64_t s) { dist[s] = round; });
                mode = 1;
            }
            else if (round >= toggle::log2(G.n()) * 2) {
                frontier.consume_each([&](uint64_t s) { dist[s] = round; next_frontier.insert(s); });
                mode = 5; round--;
            }
            else {
                frontier.consume_each([&](uint64_t s) {
                    dist[s] = round;
                    toggle::parallel_for(G.out_offset(s), G.out_offset(s+1), [&](uint64_t i) {
                        uint64_t d = G.out_edge(i);
                        if (active.contains(d)) {
                            active.remove(d); next_frontier.insert(d);
                        }
                    }, 256);
                });
            }
        }
        else if (mode == 1) {
            active.for_each([&](uint64_t s) {
                for (uint64_t i = G.in_offset(s), ie = G.in_offset(s+1); i < ie; i++) {
                    uint64_t d = G.in_edge(i);
                    if (!active.contains(d)) {
                        next_frontier.insert(s);
                        return;
                    }
                }
            });
            std::swap(frontier, next_frontier);
            if (frontier.empty()) break;
            if (frontier.num_elements() < (G.n() >> 6)) {
                frontier.consume_each([&](uint64_t s) { next_frontier.insert(s); active.remove(s); });
                round--; mode = 0;
            }
            else {
                frontier.consume_each([&](uint64_t s) { dist[s] = round; active.remove(s); });
            }
        }
        else {
            std::swap(frontier, next_frontier);
            if (frontier.empty()) break;
            if (mode % 5 == 0) {
                uint64_t new_stride = std::max<uint64_t>(1ULL, 1500000 * G.m() / (G.n() * frontier.num_elements()));
                if (stride < new_stride) stride++;
                if (stride > new_stride) stride--;
            }
            auto local_search = [&](auto&& self, uint64_t v) -> void {
                if (G.out_offset(v+1) - G.out_offset(v) >= 1024) {
                    uint64_t s = v;
                    uint32_t min_dist_d = toggle::reduce<toggle::min, uint32_t(UINT32_MAX)>(G.in_offset(s), G.in_offset(s+1), [&](uint64_t i) {
                        uint64_t d = G.in_edge(i);
                        return toggle::atomic_read(dist[d]);
                    });
                    uint64_t dist_s = min_dist_d + 1;
                    toggle::write_min(dist[s], dist_s);
                    toggle::parallel_for(G.out_offset(s), G.out_offset(s+1), [&](uint64_t i) {
                        uint64_t d = G.out_edge(i);
                        if (toggle::write_min(dist[d], dist_s + 1)) {
                            if (dist_s + 1 == round + stride) {  next_frontier.insert(d); }
                            else { self(self, d); }
                        }
                    }, 256);
                }
                else {
                    auto local_queue = toggle::LocalQueue<true>(G, 1024, v, toggle::prefetch_target(dist));
                    while (true) {
                        uint64_t s = local_queue.pop_top();
                        if (s == UINT64_MAX) { break; }
                        uint32_t min_dist_d = UINT32_MAX;
                        for (uint64_t i = G.in_offset(s), ie = G.in_offset(s + 1); i < ie; i++) {
                            uint64_t d = G.in_edge(i);
                            uint32_t dist_d = toggle::atomic_read(dist[d]);
                            if (dist_d < min_dist_d) min_dist_d = dist_d;
                        }
                        uint64_t dist_s = min_dist_d + 1;
                        toggle::write_min(dist[s], dist_s);
                        for (uint64_t i = G.out_offset(s), ie = G.out_offset(s + 1); i < ie; i++) {
                            uint64_t d = G.out_edge(i);
                            if (toggle::write_min(dist[d], dist_s + 1)) {
                                if (dist_s + 1 == round + stride) { next_frontier.insert(d); }
                                else { local_queue.push_back(d); }
                            }
                        }
                    }
                    local_queue.for_each([&](uint64_t s) { self(self, s); });
                }
            };
            frontier.consume_each([&](uint64_t s) {
                if (toggle::atomic_read(dist[s]) != round) return;
                local_search(local_search, s);
            });
            round = round + stride - 1;
            mode++;
        }
    }

    return dist;
}

template <class Graph>
toggle::Array<uint64_t> breadth_first_search_tree(Graph& G, uint64_t source = 0) {
    auto active = toggle::IndexSet(G.n(), true);
    auto frontier = toggle::IndexSet(G.n(), false);
    auto next_frontier = toggle::IndexSet(G.n(), false);
    auto reduce_edge = [&]() { return frontier.reduce<toggle::sum, uint64_t(0)>([&](uint64_t s) { return G.out_offset(s + 1) - G.out_offset(s); }); };
    auto dist_and_parent = toggle::Array<uint64_t>(G.n(), UINT64_MAX);
    next_frontier.insert(source);
    active.remove(source);
    dist_and_parent[source] = toggle::glue32(0, UINT32_MAX);
    uint64_t mode = 0, stride = 1;

    for (uint64_t round = 1; ; round++) {
        if (mode == 0) {
            std::swap(frontier, next_frontier);
            if (frontier.empty()) break;
            if (reduce_edge() > (G.m() >> 3)) {
                frontier.consume_each([](uint64_t) { });
                mode = 1; round--;
            }
            else if (round >= toggle::log2(G.n()) * 2) {
                frontier.consume_each([&](uint64_t s) { next_frontier.insert(s); });
                mode = 5; round -= 2;
            }
            else {
                frontier.consume_each([&](uint64_t s) {
                    toggle::parallel_for(G.out_offset(s), G.out_offset(s+1), [&](uint64_t i) {
                        uint64_t d = G.out_edge(i);
                        if (toggle::write_min(dist_and_parent[d], toggle::glue32(round, s))) {
                            active.remove(d); next_frontier.insert(d);
                        }
                    }, 256);
                });
            }
        }
        else if (mode == 1) {
            active.for_each([&](uint64_t s) {
                for (uint64_t i = G.in_offset(s), ie = G.in_offset(s+1); i < ie; i++) {
                    uint64_t d = G.in_edge(i);
                    if (!active.contains(d)) {
                        next_frontier.insert(s);
                        dist_and_parent[s] = toggle::glue32(round, d);
                        return;
                    }
                }
            });
            std::swap(frontier, next_frontier);
            if (frontier.empty()) break;
            if (frontier.num_elements() < (G.n() >> 6)) {
                frontier.consume_each([&](uint64_t s) { next_frontier.insert(s); active.remove(s); });
                mode = 0;
            }
            else {
                frontier.consume_each([&](uint64_t s) { active.remove(s); });
            }
        }
        else {
            std::swap(frontier, next_frontier);
            if (frontier.empty()) break;
            if (mode % 5 == 0) {
                uint64_t new_stride = std::max<uint64_t>(1ULL, 1500000 * G.m() / (G.n() * frontier.num_elements()));
                if (stride < new_stride) stride++;
                if (stride > new_stride) stride--;
            }
            auto local_search = [&](auto&& self, uint64_t v) -> void {
                if (G.out_offset(v+1) - G.out_offset(v) >= 1024) {
                    uint64_t s = v;
                    std::pair<uint32_t, uint32_t> aggregate = toggle::reduce<toggle::min, std::pair<uint32_t, uint32_t>{UINT32_MAX, UINT32_MAX}>(G.in_offset(s), G.in_offset(s+1), [&](uint64_t i) {
                        uint64_t d = G.in_edge(i);
                        return std::pair<uint32_t, uint32_t>{toggle::hi32(toggle::atomic_read(dist_and_parent[d])), d};
                    });
                    uint32_t dist_s = aggregate.first + 1;
                    toggle::write_min(dist_and_parent[s], toggle::glue32(dist_s, aggregate.second));
                    toggle::parallel_for(G.out_offset(s), G.out_offset(s+1), [&](uint64_t i) {
                        uint64_t d = G.out_edge(i);
                        if (toggle::write_min(dist_and_parent[d], toggle::glue32(dist_s + 1, s))) {
                            if (dist_s + 1 == round + stride) { next_frontier.insert(d); }
                            else { self(self, d); }
                        }
                    }, 256);
                }
                else {
                    auto local_queue = toggle::LocalQueue<true>(G, 1024, v, toggle::prefetch_target(dist_and_parent));
                    while (true) {
                        uint64_t s = local_queue.pop_top();
                        if (s == UINT64_MAX) { break; }
                        uint32_t min_dist_d = UINT32_MAX; uint32_t min_d = UINT32_MAX;
                        for (uint64_t i = G.in_offset(s), ie = G.in_offset(s+1); i < ie; i++) {
                            uint64_t d = G.in_edge(i);
                            uint32_t dist_d = toggle::hi32(toggle::atomic_read(dist_and_parent[d]));
                            if (dist_d < min_dist_d) {
                                min_dist_d = dist_d;
                                min_d = d;
                            }
                            else if (dist_d == min_dist_d && d < min_d) {
                                min_d = d;
                            }
                        }
                        uint64_t dist_s = min_dist_d + 1;
                        toggle::write_min(dist_and_parent[s], toggle::glue32(dist_s, min_d));
                        for (uint64_t i = G.out_offset(s), ie = G.out_offset(s+1); i < ie; i++) {
                            uint64_t d = G.out_edge(i);
                            if (toggle::write_min(dist_and_parent[d], toggle::glue32(dist_s + 1, s))) {
                                if (dist_s + 1 == round + stride) { next_frontier.insert(d); }
                                else { local_queue.push_back(d); }
                            }
                        }
                    }
                    local_queue.for_each([&](uint64_t s) { self(self, s); });
                }
            };
            frontier.consume_each([&](uint64_t s) {
                if (toggle::hi32(toggle::atomic_read(dist_and_parent[s])) != round) return;
                local_search(local_search, s);
            });
            round = round + stride - 1;
            mode++;
        }
    }

    return dist_and_parent;
}
} // namespace toggle
