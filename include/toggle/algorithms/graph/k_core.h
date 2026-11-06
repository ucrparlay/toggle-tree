// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstddef>
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
#include "../../primitives/pack.h"

namespace toggle {

template <class Graph>
toggle::Array<uint32_t> k_core_decomposition(Graph& G) {
    auto coreness = toggle::Array<uint32_t>(G.n());
    auto frontier = toggle::IndexSet(G.n(), false);
    auto next_frontier = toggle::IndexSet(G.n(), false);
    auto active = toggle::IndexSet(G.n(), true);
    auto resample = toggle::IndexSet(G.n(), false);
    auto sample_mode = toggle::IndexSet(G.n(), false);
    uint64_t workers = toggle::num_workers();
    uint32_t sample_threshold = workers >= 2 ? 8192 : UINT32_MAX;
    uint32_t k = 0;
    uint32_t no_reduce = 2 * toggle::log2(G.n());

    auto peel = [&](uint32_t d) -> bool {
        uint32_t old = toggle::fetch_and_sub(coreness[d], 1);
        if (old <= k) [[unlikely]] { toggle::fetch_and_add(coreness[d], 1); }
        return old == k + 1;
    };
    toggle::parallel_for(0, G.n(), [&](size_t i) {
        coreness[i] = G.offset(i + 1) - G.offset(i);
        if (coreness[i] == 0) { active.remove(i); }
    }, 1024);
    auto high = toggle::pack<uint32_t>(0, G.n(),
        [&](uint64_t i) { return coreness[i] > sample_threshold; },
        [&](uint64_t i) { return uint32_t(i); }
    );
    auto place = toggle::Array<uint32_t>(G.n());
    auto shards = toggle::Array<toggle::Array<int64_t>>(workers, [&](uint64_t) {
        return toggle::Array<int64_t>(high.size());
    }, 1);
    auto distribute = [&](uint32_t s, int64_t remaining) {
        uint64_t p = place[s];
        int64_t quotient = remaining / int64_t(workers);
        uint64_t remainder = remaining % int64_t(workers);
        for (uint64_t w = 0; w < workers; ++w) {
            shards[w][p] = quotient + (w < remainder);
        }
    };
    auto total_remaining = [&](uint32_t s) {
        int64_t total = 0;
        uint64_t p = place[s];
        for (uint64_t w = 0; w < workers; w++) total += shards[w][p];
        return total;
    };
    toggle::parallel_for(0, high.size(), [&](uint64_t p) {
        auto s = high[p];
        place[s] = p;
        sample_mode.insert(s);
        distribute(s, G.offset(s+1) - G.offset(s));
    }, 1);

    while (!active.empty()) {
        uint32_t delta = no_reduce == 0 ? active.reduce<toggle::min, uint32_t(UINT32_MAX)>([&](size_t i) { return coreness[i]; }) - k : 1;
        k += delta;
        auto down = sample_mode.reduce<toggle::max, int64_t(0)>([&](uint64_t s) {
            shards[toggle::worker_id()][place[s]] -= delta;
            if (shards[toggle::worker_id()][place[s]] <= 0) {
                int64_t remaining = total_remaining(s);
                if (remaining > int64_t(sample_threshold)) {
                    distribute(s, remaining);
                    return int64_t(0);
                }
                else {
                    coreness[s] = uint32_t(remaining + k);
                    sample_mode.remove(s);
                    return remaining < 0 ? -remaining : int64_t(0);
                }
            }
            else { return int64_t(0); }
        });
        if (down != 0) {
            k -= uint32_t(down);
            sample_mode.for_each([&](uint64_t s) { shards[toggle::worker_id()][place[s]] += down; });
        }

        active.for_each([&](uint32_t s) {
            if (coreness[s] == k) {
                frontier.insert(s);
                active.remove(s);
            }
        });

        if (no_reduce > 0 && frontier.empty()) { no_reduce--; }

        while (!frontier.empty()) {
            auto local_search = [&](auto&& self, uint32_t v, uint32_t layer) -> void {
                if (G.offset(v+1) - G.offset(v) >= 128) {
                    uint32_t s = v;
                    toggle::parallel_for(G.offset(s), G.offset(s+1), [&](size_t i) {
                        uint32_t d = G.edge(i);
                        if (active.contains(d)) {
                            if (sample_mode.contains(d)) {
                                if (--shards[toggle::worker_id()][place[d]] == 0) { resample.insert(d); }
                            }
                            else {
                                if (peel(d)) {
                                    active.remove(d);
                                    if (layer == 0) next_frontier.insert(d);
                                    else self(self, d, layer - 1);
                                }
                            }
                        }
                    }, 128);
                }
                else {
                    auto local_queue = toggle::LocalQueue(G, 128, v);
                    while (true) {
                        uint64_t s = local_queue.pop_top();
                        if (s == UINT64_MAX) { break; }
                        for (uint64_t i = G.offset(s), ie = G.offset(s+1); i < ie; i++) {
                            uint64_t d = G.edge(i);
                            if (active.contains(d)) {
                                if (!sample_mode.empty() && sample_mode.contains(d)) {
                                    if (--shards[toggle::worker_id()][place[d]] == 0) { resample.insert(d); }
                                }
                                else {
                                    if (peel(d)) {
                                        active.remove(d);
                                        local_queue.push_back(d);
                                    }
                                }
                            }
                        }
                    }
                    if (layer == 0) { local_queue.sequential_for_each([&](uint64_t s) { next_frontier.insert(s); }); }
                    else { local_queue.for_each([&](uint64_t s) { self(self, s, layer - 1); }); }
                }
            };
            frontier.consume_each([&](uint32_t s) {
                local_search(local_search, s, 8);
            });
            resample.consume_each([&](uint32_t s) {
                int64_t remaining = total_remaining(s);
                if (remaining > int64_t(sample_threshold)) {
                    distribute(s, remaining);
                } else {
                    coreness[s] = uint32_t(std::max<int64_t>(k, remaining + k));
                    sample_mode.remove(s);
                    if (remaining <= 0) {
                        active.remove(s);
                        next_frontier.insert(s);
                    }
                }
            });
            std::swap(frontier, next_frontier);
        }
    }
    return coreness;
}

} // namespace toggle
