#pragma once
#include "graph.h"

struct EdgeBlock {
        uint32_t v;
        size_t start;
        size_t end;
    };

template <class Graph, class Frontier, class Cond, class Update>
void EdgeMapBlocked(
    Graph& G,
    Frontier& frontier,
    Cond&& cond,
    Update&& update,
    size_t BLOCK_SIZE = 128
) {
    size_t n = frontier.size();

    // ===== 1. nb[i] = frontier[i] 需要多少 blocks =====
    auto nb = parlay::sequence<size_t>(n);
    parlay::parallel_for(0, n, [&](size_t i) {
        uint32_t v = frontier[i];
        size_t deg = G.offsets[v + 1] - G.offsets[v];
        nb[i] = (deg + BLOCK_SIZE - 1) / BLOCK_SIZE;
    });

    // ===== 2. offs[i] = frontier[i] 对应的blocks的编号起点 =====
    auto offs = parlay::sequence<size_t>(n + 1);
    offs[0] = 0;
    for (size_t i = 0; i < n; i++) { offs[i + 1] = offs[i] + nb[i]; }
    size_t num_blocks = offs[n];

    // ===== 3. 构造 block 表，存下要访问的点在CSR中的起始位置和结束位置 =====
    
    auto blocks = parlay::sequence<EdgeBlock>(num_blocks);
    parlay::parallel_for(0, n, [&](size_t i) {
        uint32_t v = frontier[i];
        size_t base = G.offsets[v];
        size_t last = G.offsets[v + 1];
        size_t off = offs[i];
        for (size_t b = 0; b < nb[i]; b++) {
            size_t s = base + b * BLOCK_SIZE;
            size_t e = std::min(s + BLOCK_SIZE, last);
            blocks[off + b] = EdgeBlock{v, s, e};
        }
    });

    // ===== 4. block 级并行 =====
    parlay::parallel_for(0, num_blocks, [&](size_t i) {
        const auto& blk = blocks[i];
        for (size_t e = blk.start; e < blk.end; ++e) {
            uint32_t d = G.edges[e].v;
            if (cond(d)) {
                update(d);
            }
        }
    });
}

template <class Graph>
class EdgeBlocks {
public:
    using block_type = EdgeBlock;

    EdgeBlocks(
        Graph& G,
        const parlay::sequence<uint32_t>& frontier,
        size_t BLOCK_SIZE = 128
    ) {
        build(G, frontier, BLOCK_SIZE);
    }

    // block 数组
    const parlay::sequence<block_type>& blocks() const {
        return blocks_;
    }

    // block 数量
    size_t size() const {
        return blocks_.size();
    }

    // 下标访问（方便 range-for / manual loop）
    const block_type& operator[](size_t i) const {
        return blocks_[i];
    }

    size_t block_begin(size_t i) const { return offs[i]; }
    size_t block_end(size_t i)   const { return offs[i + 1]; }

private:
    parlay::sequence<block_type> blocks_;
    parlay::sequence<size_t> offs;

    void build(
        Graph& G,
        const parlay::sequence<uint32_t>& frontier,
        size_t BLOCK_SIZE
    ) {
        size_t n = frontier.size();

        // ===== 1. 每个 frontier 点需要多少 blocks =====
        auto nb = parlay::sequence<size_t>(n);
        parlay::parallel_for(0, n, [&](size_t i) {
            uint32_t v = frontier[i];
            size_t deg = G.offsets[v + 1] - G.offsets[v];
            nb[i] = (deg + BLOCK_SIZE - 1) / BLOCK_SIZE;
        });

        // ===== 2. block 前缀和 =====
        offs = parlay::sequence<size_t>(n + 1);
        offs[0] = 0;
        for (size_t i = 0; i < n; i++) {
            offs[i + 1] = offs[i] + nb[i];
        }

        size_t num_blocks = offs[n];
        blocks_ = parlay::sequence<block_type>(num_blocks);

        // ===== 3. 填 block 表 =====
        parlay::parallel_for(0, n, [&](size_t i) {
            uint32_t v = frontier[i];
            size_t base = G.offsets[v];
            size_t last = G.offsets[v + 1];
            size_t off  = offs[i];

            for (size_t b = 0; b < nb[i]; b++) {
                size_t s = base + b * BLOCK_SIZE;
                size_t e = std::min(s + BLOCK_SIZE, last);
                blocks_[off + b] = block_type{v, s, e};
            }
        });
    }
};


template <typename F>
inline void adaptive_for(
    size_t start,
    size_t end,
    F&& f,
    size_t threshold = 256,
    size_t granularity = 128
) {
    if (end - start < threshold) {
        for (size_t i = start; i < end; ++i) {
            f(i);
        }
    } 
    else {
        parlay::parallel_for(start, end, std::forward<F>(f), granularity);
    }
}

template <class Graph, class F>
inline uint32_t adaptive_sum(Graph& G, uint32_t s, size_t l, size_t r, F&& f) {
    if (r - l < 256) {
        uint32_t cnt = 0;
        for (size_t j = l; j < r; j++) {
            uint32_t d = G.edges[j].v;
            cnt += f(d);
        }
        return cnt;
    }
    size_t m = ((l + r) >> 5) << 4;
    uint32_t left, right;
    parlay::parallel_do(
        [&]() { left  = adaptive_sum(G, s, l, m, f); },
        [&]() { right = adaptive_sum(G, s, m, r, f); }
    );
    return left + right;
}

template <class Graph, class F>
inline uint32_t adaptive_min(Graph& G, uint32_t s, size_t l, size_t r, F&& f) {
    if (r - l < 256) {
        uint32_t result = UINT32_MAX;
        for (size_t j = l; j < r; j++) {
            uint32_t d = G.edges[j].v;
            uint32_t fd = f(d);
            if (result > fd) result = fd;
        }
        return result;
    }
    size_t m = ((l + r) >> 5) << 4;
    uint32_t left, right;
    parlay::parallel_do(
        [&]() { left  = adaptive_min(G, s, l, m, f); },
        [&]() { right = adaptive_min(G, s, m, r, f); }
    );
    return std::min(left, right);
}

template <class Graph, class F>
inline bool adaptive_exist(Graph& G, uint32_t s, size_t l, size_t r, F&& f) {
    if (r - l < 256) {
        for (size_t j = l; j < r; j++) {
            uint32_t d = G.edges[j].v;
            if (f(d)) return true;
        }
        return false;
    }
    size_t m = ((l + r) >> 5) << 4;
    bool left, right;
    parlay::parallel_do(
        [&]() { left  = adaptive_exist(G, s, l, m, f); },
        [&]() { right = adaptive_exist(G, s, m, r, f); }
    );
    return left || right;
}

template <typename F>
inline void adaptive_for2( size_t start, size_t end, F&& f ) {
    if (end - start < 256) {
        for (size_t i = start; i < end; ++i) { f(i); }
        return;
    } 
    size_t m = ((start + end) >> 5) << 4;
    parlay::parallel_do(
        [&]() { adaptive_for2(start, m, f); },
        [&]() { adaptive_for2(m, end, f); }
    );
}

static inline bool write_min(uint32_t& ref,uint32_t v){
    uint32_t old=__atomic_load_n(&ref,__ATOMIC_RELAXED);
    if(old==v)return false;
    while(old>v&&!__atomic_compare_exchange_n(&ref,&old,v,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED));
    return old>v;
}

template <class Graph, class Frontier, class Cond, class Update>
void EdgeMapSparse(
    Graph& G,
    Frontier& frontier,
    Cond&& cond,
    Update&& update,
    size_t BLOCK_SIZE = 128
) {
    parlay::parallel_for(0, frontier.size(), [&](size_t _) { 
        uint32_t s = frontier[_]; 
        size_t start = G.offsets[s];
        size_t end = G.offsets[s + 1];
        adaptive_for(start, end, [&] (size_t j) {
            uint32_t d = G.edges[j].v;
            if (cond(d)) {
                update(d);
            }
        });
    });
}
        
        