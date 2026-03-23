#pragma once
#include <cassert>
#include <fstream>
#include <string>
#include <type_traits>
#include <utility>
#include <iostream>
#include <parlay/sequence.h>

struct Empty {};

template <class Wgh>
struct Edge {
    uint32_t v;
    [[no_unique_address]] Wgh w;
    Edge() {}
    Edge(uint32_t _v) : v(_v) {}
    Edge(uint32_t _v, Wgh _w) : v(_v), w(_w) {}
};

template <class Wgh = Empty>
struct Graph {
    static_assert(std::is_same_v<Wgh, Empty> || std::is_same_v<Wgh, int32_t>, "Wgh must be Empty or int32_t");

    std::string name;
    size_t n;
    size_t m;
    bool symmetrized;
    bool weighted;
    parlay::sequence<uint64_t> offsets;
    parlay::sequence<Edge<Wgh>> edges;
    parlay::sequence<uint64_t>& in_offsets;
    parlay::sequence<Edge<Wgh>>& in_edges;
    parlay::sequence<uint64_t> in_offsets_seq;
    parlay::sequence<Edge<Wgh>> in_edges_seq;

    Graph(const char *filename, int32_t l=1, int32_t r=1): 
        symmetrized(std::string(filename).find("_sym") != std::string::npos),
        weighted(false),
        in_offsets(symmetrized ? offsets : in_offsets_seq),
        in_edges(symmetrized ? edges : in_edges_seq)
    {
        std::string str_filename(filename);
        std::string subfix = str_filename.substr(str_filename.find_last_of('.') + 1);
        if (subfix == "bin") {
            read_binary_format(filename);
        } 
        else if (subfix == "adj") {
            read_pbbs_format(filename);
        } 
        else {
            std::cerr << "Error: Invalid graph extension or format: " << filename << "\n"; abort();
        }
        name = str_filename.substr(0, str_filename.find_last_of('.'));
        if (name.find_last_of('/') != std::string::npos) { name = name.substr(name.find_last_of('/') + 1); }
        if (!symmetrized) { make_inverse(); }
        if constexpr (std::is_same_v<Wgh, int32_t>) { if (!weighted) make_random_weight(l, r); }
    }

    void make_inverse() {
        parlay::sequence<std::pair<uint32_t, Edge<Wgh>>> edgelist(m);
        parlay::parallel_for(0, n, [&](uint32_t u) {
            parlay::parallel_for(offsets[u], offsets[u + 1], [&](uint64_t i) {
                edgelist[i] = std::make_pair(edges[i].v, Edge<Wgh>(u, edges[i].w));
            });
        });
        parlay::sort_inplace(parlay::make_slice(edgelist), [](auto &a, auto &b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second.v < b.second.v;
        });
        in_offsets_seq = parlay::sequence<uint64_t>(n + 1, m);
        in_edges_seq = parlay::sequence<Edge<Wgh>>(m);
        parlay::parallel_for(0, m, [&](size_t i) {
            in_edges_seq[i] = edgelist[i].second;
            if (i == 0 || edgelist[i].first != edgelist[i - 1].first) {
                in_offsets_seq[edgelist[i].first] = i;
            }
        });
        parlay::scan_inclusive_inplace(parlay::make_slice(in_offsets_seq.rbegin(), in_offsets_seq.rend()), parlay::minm<uint64_t>());
    }

    void make_random_weight(int32_t l, int32_t r) {
        weighted = true;
        int32_t range = r - l + 1;
        parlay::parallel_for(0, n, [&](uint32_t u) {
            parlay::parallel_for(offsets[u], offsets[u + 1], [&](uint64_t i) {
                uint32_t v = edges[i].v;
                edges[i].w = ((parlay::hash32(u) ^ parlay::hash32(v)) % range) + l;
            });
        });
        if (!symmetrized) {
            parlay::parallel_for(0, n, [&](uint32_t v) {
                for (uint64_t j = in_offsets_seq[v]; j < in_offsets_seq[v + 1]; j++) {
                    uint32_t u = in_edges_seq[j].v;
                    in_edges_seq[j].w = ((parlay::hash32(u) ^ parlay::hash32(v)) % range) + l;
                }
            });
        }
    }

    void read_binary_format(char const *filename) {
        struct stat sb;
        int fd = open(filename, O_RDONLY);
        if (fd == -1 || fstat(fd, &sb) == -1) { std::cerr << "Error: Failed in opening file " << filename << "\n"; abort(); }
        char *data = static_cast<char *>(mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        size_t len = sb.st_size;
        n = reinterpret_cast<uint64_t *>(data)[0];
        m = reinterpret_cast<uint64_t *>(data)[1];
        if (n == 0) { std::cerr << "Error: Failed in reading graph.\n"; abort(); }
        assert(reinterpret_cast<uint64_t *>(data)[2] == (n + 1) * 8 + m * 4 + 3 * 8 && "File size mismatch for out-edge binary");
        offsets = parlay::sequence<uint64_t>::uninitialized(n + 1);
        edges = parlay::sequence<Edge<Wgh>>::uninitialized(m);
        parlay::parallel_for(0, n + 1, [&](size_t i) {
            offsets[i] = reinterpret_cast<uint64_t *>(data + 3 * 8)[i];
        });
        parlay::parallel_for(0, m, [&](size_t i) {
            edges[i].v = reinterpret_cast<uint32_t *>(data + 3 * 8 + (n + 1) * 8)[i];
        });
        munmap(data, len);
        close(fd);
    }

    void read_pbbs_format(char const *filename) {
        struct stat sb; int fd = open(filename, O_RDONLY);
        if (fd == -1 || fstat(fd, &sb) == -1) { std::cerr << "Error: Failed in opening file " << filename << "\n"; abort(); }
        char *data = (char*)mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0); if (data == MAP_FAILED) { std::cerr << "Error: mmap failed " << filename << "\n"; abort(); }
        madvise(data, sb.st_size, MADV_SEQUENTIAL); char *ed = data + sb.st_size, *p = data;
        auto issp = [&](char c) -> bool { return (unsigned char)c <= ' '; };
        auto skip = [&](char *&x) { while (x < ed && issp(*x)) ++x; };
        auto next_tok = [&](char *&x, char *lim) -> std::pair<char*,char*> { while (x < lim && issp(*x)) ++x; char *l = x; while (x < lim && !issp(*x)) ++x; return {l, x}; };
        auto parse_u64 = [&](char *l, char *r) -> uint64_t { uint64_t x = 0; while (l < r) x = x * 10 + (uint64_t)(*l++ - '0'); return x; };
        auto parse_u32 = [&](char *l, char *r) -> uint32_t { uint32_t x = 0; while (l < r) x = x * 10 + (uint32_t)(*l++ - '0'); return x; };
        auto parse_w = [&](char *l, char *r) -> Wgh { if constexpr (std::is_same_v<Wgh, Empty>) return Wgh(); else { bool neg = false; if (l < r && *l == '-') neg = true, ++l; int64_t x = 0; while (l < r) x = x * 10 + (*l++ - '0'); return (Wgh)(neg ? -x : x); } };
        auto [h0,h1] = next_tok(p, ed); std::string_view header(h0, h1 - h0);
        auto [n0,n1] = next_tok(p, ed); n = parse_u64(n0, n1); auto [m0,m1] = next_tok(p, ed); m = parse_u64(m0, m1);
        if (!n) { std::cerr << "Error: Failed in reading graph.\n"; abort(); }
        if (header == "WeightedAdjacencyGraph") weighted = true; else if (header == "AdjacencyGraph") weighted = false; else { std::cerr << "Unrecognized PBBS format\n"; abort(); }
        offsets = parlay::sequence<uint64_t>::uninitialized(n + 1); edges = parlay::sequence<Edge<Wgh>>::uninitialized(m); skip(p); char *body = p;
        uint64_t need = n + m + (weighted ? m : 0); int T = std::max(1, 4 * (int)parlay::num_workers()); if ((uint64_t)T > need) T = (int)need; if (T <= 0) T = 1;
        parlay::sequence<char*> L(T + 1); L[0] = body; L[T] = ed; size_t body_len = ed - body;
        for (int t = 1; t < T; ++t) { char *x = body + ((__uint128_t)body_len * t / T); while (x < ed && !issp(*x)) ++x; while (x < ed && issp(*x)) ++x; L[t] = x; }
        parlay::sequence<uint64_t> cnt(T), pref(T + 1); pref[0] = 0;
        for (int t = 0; t < T; ++t) { char *x = L[t], *r = L[t + 1]; uint64_t c = 0; while (1) { while (x < r && issp(*x)) ++x; if (x >= r) break; while (x < r && !issp(*x)) ++x; ++c; } cnt[t] = c; pref[t + 1] = pref[t] + c; }
        if (pref[T] != need) { std::cerr << "Error: token count mismatch, got " << pref[T] << " expected " << need << "\n"; abort(); }
        parlay::parallel_for(0, T, [&](int t) { char *x = L[t], *r = L[t + 1]; uint64_t k = pref[t]; while (1) { while (x < r && issp(*x)) ++x; if (x >= r) break; char *l = x; while (x < r && !issp(*x)) ++x; if (k < n) offsets[k] = parse_u64(l, x); else if (k < n + m) edges[k - n].v = parse_u32(l, x); else { if constexpr (std::is_same_v<Wgh, Empty>) {} else edges[k - n - m].w = parse_w(l, x); } ++k; } }, 1);
        offsets[n] = m; munmap(data, sb.st_size); close(fd);
    }

    void write_pbbs_format(char const *filename) {
        std::ofstream out(filename);
        if (!out) { std::cerr << "Error: Cannot open output file " << filename << "\n"; abort(); }
        out << "WeightedAdjacencyGraph\n";
        out << n << '\n';
        out << m << '\n';
        for (size_t i = 0; i < n; i++) out << offsets[i] << '\n';
        for (size_t i = 0; i < m; i++) out << edges[i].v << '\n';
        for (size_t i = 0; i < m; i++) out << edges[i].w << '\n';
    }

};