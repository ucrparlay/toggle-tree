// SPDX-License-Identifier: MIT
#pragma once
#include <bit>
#include <cstdint>
#include <utility>
#include "../../parallel/parallel.h"
#include "../../parallel/atomic.h"
#include "../../data_structures/array.h"
#include "../../data_structures/index_set.h"
#include "../../primitives/combine.h"
#include "../../primitives/reduce.h"
#include "../../primitives/random.h"

namespace toggle {

template <class Graph>
toggle::Array<uint32_t> graph_coloring(Graph& G) {
    struct alignas(64) Palette { uint64_t w[8]; };
    constexpr auto color_mix = [](Palette a, Palette b) {
        Palette c;
        for (int i = 0; i < 8; i++) c.w[i] = a.w[i] | b.w[i];
        return c;
    };
    auto pigment = [](uint32_t c) {
        Palette r{};
        r.w[c >> 6] = 1ULL << (c & 63);
        return r;
    };

    auto color = toggle::Array<uint32_t>(G.n(), UINT32_MAX);
    auto frontier = toggle::IndexSet(G.n(), false);
    auto next_frontier = toggle::IndexSet(G.n(), false);
    auto num_blockers = toggle::Array<uint32_t>(G.n());
    auto used_color = toggle::Array<bool>(G.m(), false);

    toggle::parallel_for(0, G.n(), [&](uint64_t s){
        uint64_t deg_s = G.offset(s+1) - G.offset(s);
        if (deg_s == 0) { color[s] = 0; return; }
        num_blockers[s] = toggle::reduce<toggle::sum, uint32_t(0)>(G.offset(s), G.offset(s + 1), [&](uint64_t i) {
            uint64_t d = G.edge(i);
            uint64_t deg_d = G.offset(d + 1) - G.offset(d);
            return (deg_d > deg_s) || (deg_d == deg_s && toggle::hash32(d) < toggle::hash32(s));
        });
        if (num_blockers[s] == 0) { frontier.insert(s); }
    }, 1);

    while (!frontier.empty()) {
        frontier.consume_each([&](uint64_t s){
            uint64_t start = G.offset(s);
            uint64_t end = G.offset(s + 1);
            uint64_t deg = end - start;
            auto used = toggle::reduce<color_mix, Palette{}>(start, end, [&](uint64_t i) {
                uint32_t d = G.edge(i);
                uint32_t color_d = color[d];
                if (color_d == UINT32_MAX) {
                    if (toggle::fetch_and_sub(num_blockers[d], 1) == 1) next_frontier.insert(d);
                    return Palette{};
                }
                if (color_d < 512) return pigment(color_d);
                if (color_d < deg) { toggle::atomic_write(used_color[start + color_d], 1); }
                return Palette{};
            }, 256);
            for (uint32_t w = 0; w < 8; ++w) {
                if (used.w[w] != UINT64_MAX) { color[s] = w * 64 + std::countr_zero(~used.w[w]); return; }
            }
            for (uint64_t i = 512; i < deg; i++) {
                if (used_color[start + i] == 0) { color[s] = i; return; }
            }
            color[s] = deg;
        });
        std::swap(frontier, next_frontier);
    }

    return color;
}

} // namespace toggle
