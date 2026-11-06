// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <tuple>
#include <type_traits>
#include "../parallel/parallel.h"
#include "array.h"
#include "index_set.h"
#include "../primitives/combine.h"

namespace toggle {

template <class... T>
struct PrefetchTarget { std::tuple<T&...> t; };

template <class... T>
inline auto prefetch_target(T&... t) noexcept {
    return PrefetchTarget<T...>{std::tie(t...)};
}

template <bool PrefetchIn, bool Direction, class Graph, class PT = PrefetchTarget<>>
struct LocalQueueImpl {
    static constexpr uint64_t data_ahead = 2, edges_ahead = 6, data_cap = 16;

    Graph& G;
    PT pf_target;
    uint64_t lpos, rpos, capacity;
    toggle::Array<uint32_t> local_queue;

    LocalQueueImpl(Graph& graph_, uint64_t capacity_, uint64_t s_, PT pf_target_ = {})
        : G(graph_), pf_target(pf_target_), lpos(0), rpos(1), capacity(capacity_ - 1), local_queue(capacity_) {
        local_queue[0] = uint32_t(s_);
    }

    void prefetch_neighbors(uint64_t p) noexcept {
        for (uint64_t i = G.template out_offset<Direction>(p), e = toggle::min(G.template out_offset<Direction>(p + 1), i + data_cap); i < e; i++) {
            uint64_t d = G.template out_edge<Direction>(i);
            std::apply([&](auto&... a) { (a.prefetch(d), ...); }, pf_target.t);
        }
        if constexpr (PrefetchIn && !Graph::symmetrized) for (uint64_t i = G.template in_offset<Direction>(p), e = toggle::min(G.template in_offset<Direction>(p + 1), i + data_cap); i < e; i++) {
            uint64_t d = G.template in_edge<Direction>(i);
            std::apply([&](auto&... a) { (a.prefetch(d), ...); }, pf_target.t);
        }
    }

    uint64_t pop_top() noexcept {
        if (lpos == rpos) { return UINT64_MAX; }
        uint64_t s = local_queue[lpos];
        uint64_t workload = G.template out_offset<Direction>(s + 1) - G.template out_offset<Direction>(s);
        if (capacity < workload) { return UINT64_MAX; }
        lpos++; capacity -= workload;
        if constexpr (std::tuple_size_v<decltype(pf_target.t)> != 0) {
            if (lpos + data_ahead < rpos) { prefetch_neighbors(local_queue[lpos + data_ahead]); }
        }
        if (lpos + edges_ahead < rpos) {
            uint64_t p = local_queue[lpos + edges_ahead];
            G.template prefetch_out_edge<Direction>(G.template out_offset<Direction>(p));
            if constexpr (PrefetchIn && !Graph::symmetrized) G.template prefetch_in_edge<Direction>(G.template in_offset<Direction>(p));
        }
        return s;
    }

    void push_back(uint64_t d) noexcept {
        G.template prefetch_out_offset<Direction>(d);
        if constexpr (PrefetchIn && !Graph::symmetrized) G.template prefetch_in_offset<Direction>(d);
        local_queue[rpos++] = uint32_t(d);
    }

    template <class F> void for_each(F&& f) {
        uint32_t* data = local_queue.data_; uint64_t l = lpos, r = rpos;
        toggle::parallel_for(l, r, [&f, data](uint64_t i) { f(data[i]); }, 1);
    }

    template <class F> void sequential_for_each(F&& f) {
        for (uint64_t i = lpos; i < rpos; i++) { f(local_queue[i]); }
    }
};

template <bool PrefetchIn = false, bool Direction = true, class Graph, class PT = PrefetchTarget<>>
inline auto LocalQueue(Graph& G, uint64_t capacity, uint64_t s, PT pf_target = {}) {
    return LocalQueueImpl<PrefetchIn, Direction, Graph, PT>(G, capacity, s, pf_target);
}

} // namespace toggle
