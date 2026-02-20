#pragma once
#include "graph.h"

template <class Graph, class Seq>
inline uint32_t color(Graph& G, uint32_t v, Seq& colors) {
    uint32_t deg = G.offsets[v + 1] - G.offsets[v];
    if (deg == 0) return 0;

    // 1. bits 分配（和 GBBS 一样）
    uint8_t* bits;
    uint8_t s_bits[1000];
    if (deg > 1000)
        bits = new uint8_t[deg];
    else
        bits = s_bits;

    constexpr size_t kDefaultGranularity = 1024;

    // 2. 并行清零（chunked）
    parlay::parallel_for(
        0, deg,
        [&](size_t i) { bits[i] = 0; },
        kDefaultGranularity);

    size_t start = G.offsets[v];
    size_t end = G.offsets[v + 1];

    // 3. 并行 map 邻居（关键：chunk 并行）
    parlay::parallel_for(
        0, end - start,
        [&](size_t j) {
            uint32_t ngh = G.edges[start + j].v;
            uint32_t c = colors[ngh];
            if (c < deg) bits[c] = 1;
        },
        kDefaultGranularity);

    // 4. 找最小可用颜色
    uint32_t chosen = UINT32_MAX;
    for (uint32_t i = 0; i < deg; ++i) {
        if (bits[i] == 0) {
        chosen = i;
        break;
        }
    }

    if (deg > 1000) delete[] bits;
    return (chosen == UINT32_MAX) ? (deg + 1) : chosen;
}



template <class Graph, class Seq>
inline uint32_t seq_color(Graph& G, uint32_t v, Seq& colors) {
    uint32_t deg = G.offsets[v + 1] - G.offsets[v];
    if (deg == 0) return 0;
    uint8_t* bits;
    uint8_t s_bits[1000];
    if (deg > 1000) {
        bits = new uint8_t[deg];
    } else {
        bits = s_bits;
    }
    for (uint32_t i = 0; i < deg; ++i) {
        bits[i] = 0;
    }
    size_t start = G.offsets[v];
    size_t end   = G.offsets[v + 1];
    for (size_t e = start; e < end; ++e) {
        uint32_t ngh = G.edges[e].v;
        uint32_t c = colors[ngh];
        if (c < deg) {
            bits[c] = 1; 
        }
    }
    uint32_t chosen = UINT32_MAX;
    for (uint32_t i = 0; i < deg; ++i) {
        if (bits[i] == 0) {
            chosen = i;
            break;
        }
    }
    if (deg > 1000) {
        delete[] bits;
    }
    return (chosen == UINT32_MAX) ? (deg + 1) : chosen;
}


template <class Graph, class Seq>
inline void verify_coloring(Graph& G, Seq& colors) {
    size_t n = G.n;
    auto bad = parlay::sequence<uint8_t>(n, 0);
    parlay::parallel_for(0, n, [&](size_t u) {
        uint32_t cu = colors[u];
        size_t start = G.offsets[u];
        size_t end   = G.offsets[u + 1];
        for (size_t e = start; e < end; ++e) {
            uint32_t v = G.edges[e].v;
            if (colors[v] == cu) {
                bad[u] = 1;   // benign race（只写 1）
                break;
            }
        }
    });
    size_t conflicts = parlay::reduce(
        parlay::delayed_seq<size_t>(n, [&](size_t i) {
            return (size_t)bad[i];
        })
    );
    std::cout << "conflicts = " << conflicts << ", ";
    if (conflicts > 0) {
        std::cout << "Invalid coloring\n";
    } else {
        std::cout << "Valid coloring\n";
    }
}