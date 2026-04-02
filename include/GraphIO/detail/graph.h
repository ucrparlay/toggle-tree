// SPDX-License-Identifier: MIT
#pragma once

#include <cassert>
#include <fstream>
#include <string>
#include <type_traits>
#include <utility>
#include <iostream>
#include <parlay/sequence.h>

namespace GraphIO {

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
    static_assert(std::is_same_v<Wgh, Empty> || std::is_same_v<Wgh, int32_t> || std::is_same_v<Wgh, float>, "Wgh must be Empty or int32_t or float");

    size_t n;
    size_t m;
    bool symmetrized;
    bool weighted;
    double load_time;
    std::string name;
    parlay::sequence<uint64_t> offsets;
    parlay::sequence<Edge<Wgh>> edges;
    parlay::sequence<uint64_t>& in_offsets;
    parlay::sequence<Edge<Wgh>>& in_edges;
    parlay::sequence<uint64_t> in_offsets_seq;
    parlay::sequence<Edge<Wgh>> in_edges_seq;

    Graph(const char *filename, int32_t r=0): 
        symmetrized(std::string(filename).find("_sym") != std::string::npos),
        weighted(false),
        in_offsets(symmetrized ? offsets : in_offsets_seq),
        in_edges(symmetrized ? edges : in_edges_seq)
    {
        parlay::internal::timer t; t.start(); 
        std::string str_filename(filename);
        std::string subfix = str_filename.substr(str_filename.find_last_of('.') + 1);
        if (subfix == "bin") { read_binary_format(filename); } 
        else if (subfix == "adj") { read_pbbs_format(filename); } 
        else { std::cerr << "Error: Invalid graph extension or format: " << filename << "\n"; abort(); }
        name = str_filename.substr(0, str_filename.find_last_of('.'));
        if (name.find_last_of('/') != std::string::npos) { name = name.substr(name.find_last_of('/') + 1); }
        if constexpr (std::is_same_v<Wgh, float>) { if (!weighted) parlay::parallel_for(0, m, [&](size_t i){ edges[i].w = 1; }); weighted = true;}
        if constexpr (std::is_same_v<Wgh, int32_t>) { if (!weighted) make_random_weight(r); }
        if (!symmetrized) { make_inverse(); }
        load_time = t.stop(); 
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
            if (i == 0 || edgelist[i].first != edgelist[i - 1].first) { in_offsets_seq[edgelist[i].first] = i; }
        });
        parlay::scan_inclusive_inplace(parlay::make_slice(in_offsets_seq.rbegin(), in_offsets_seq.rend()), parlay::minm<uint64_t>());
    }


    void make_random_weight(int32_t r=0) {
        weighted = true;
        assert(r >= 0); if (r == 0) r = (int32_t)std::log2((double)n);
        int32_t range = r;
        parlay::parallel_for(0, n, [&](uint32_t u) {
            parlay::parallel_for(offsets[u], offsets[u + 1], [&](uint64_t i) {
                uint32_t v = edges[i].v;
                edges[i].w = 1 + ((parlay::hash32(u) ^ parlay::hash32(v)) % range);
            });
        });
    }

    void read_binary_format(char const *filename) {
        struct stat sb;
        int fd=open(filename,O_RDONLY);
        if(fd==-1||fstat(fd,&sb)==-1){ std::cerr<<"Error: Failed in opening file "<<filename<<"\n"; abort(); }
        char *data=(char*)mmap(0,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0);
        size_t len=sb.st_size;
        n=((uint64_t*)data)[0];
        m=((uint64_t*)data)[1];
        uint64_t bytes=((uint64_t*)data)[2];
        offsets=parlay::sequence<uint64_t>::uninitialized(n+1);
        edges=parlay::sequence<Edge<Wgh>>::uninitialized(m);
        parlay::parallel_for(0,n+1,[&](size_t i){ offsets[i]=((uint64_t*)(data+24))[i]; });
        if (bytes==24+(n+1)*8+m*4) {
            parlay::parallel_for(0,m,[&](size_t i){ edges[i].v=((uint32_t*)(data + 3 * 8 + (n + 1) * 8))[i]; });
            weighted=false;
        }
        else if (bytes==24+(n+1)*8+m*8) {
            struct X{ Wgh w; uint32_t v; }; static_assert(sizeof(X)==8);
            parlay::parallel_for(0,m,[&](size_t i){
                auto &x=((X*)(data + 3 * 8 + (n + 1) * 8))[i]; edges[i].v=x.v; edges[i].w=x.w;
            });
            weighted=true;
        }
        else { std::cerr << "Error: Incorrect file format.\n"; abort(); }
        munmap(data,len);
        close(fd);
    }

    void write_binary_format(char const *filename) const {
        std::ofstream out(filename,std::ios::binary);
        if(!out){ std::cerr<<"Error: Cannot open output file "<<filename<<"\n"; abort(); }
        uint64_t hdr[3];
        hdr[0]=n;
        hdr[1]=m;
        if constexpr(std::is_same_v<Wgh,Empty>) hdr[2]=3*8+(n+1)*8+m*4;
        else hdr[2]=3*8+(n+1)*8+m*8;
        out.write((char*)hdr,3*8);
        out.write((char*)offsets.begin(),(std::streamsize)((n+1)*8));
        if constexpr(std::is_same_v<Wgh,Empty>){
            auto to=parlay::sequence<uint32_t>::uninitialized(m);
            parlay::parallel_for(0,m,[&](size_t i){ to[i]=edges[i].v; });
            out.write((char*)to.begin(),(std::streamsize)(m*4));
        }else{
            struct X{ Wgh w; uint32_t v; }; 
            auto ew=parlay::sequence<X>::uninitialized(m);
            parlay::parallel_for(0,m,[&](size_t i){ ew[i]={edges[i].w,edges[i].v}; });
            out.write((char*)ew.begin(),(std::streamsize)(m*8));
        }
        out.close();
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
        auto parse_w = [&](char *l, char *r) -> Wgh {
            if constexpr (std::is_same_v<Wgh, Empty>) return Wgh();
            else { bool neg = false; if (l < r && (*l == '-' || *l == '+')) neg = (*l == '-'), ++l; double x = 0, frac = 0, base = 1;
            while (l < r && *l >= '0' && *l <= '9') x = x * 10 + (*l++ - '0');
            if (l < r && *l == '.') { ++l; while (l < r && *l >= '0' && *l <= '9') frac = frac * 10 + (*l++ - '0'), base *= 10; x += frac / base; }
            if (l < r && (*l == 'e' || *l == 'E')) { ++l; bool eneg = false; if (l < r && (*l == '-' || *l == '+')) eneg = (*l == '-'), ++l; int e = 0; while (l < r && *l >= '0' && *l <= '9') e = e * 10 + (*l++ - '0'); x *= std::pow(10.0, eneg ? -e : e); }
            if (neg) x = -x;
            return (Wgh)x; }
        };
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
        if constexpr (!std::is_same_v<Wgh, Empty>) out << "Weighted";
        out << "AdjacencyGraph\n" << n << '\n' << m << '\n';
        for (size_t i = 0; i < n; i++) out << offsets[i] << '\n';
        for (size_t i = 0; i < m; i++) out << edges[i].v << '\n';
        if constexpr (!std::is_same_v<Wgh, Empty>) for (size_t i = 0; i < m; i++) out << edges[i].w << '\n';
    }

    bool neighbors_sorted() const {
        parlay::sequence<uint8_t> ok(n, 1);
        parlay::parallel_for(0, n, [&](size_t u) {
            for (uint64_t i = offsets[u] + 1; i < offsets[u + 1]; ++i) if (edges[i - 1].v > edges[i].v) { ok[u] = 0; return; }
            for (uint64_t i = in_offsets[u] + 1; i < in_offsets[u + 1]; ++i) if (in_edges[i - 1].v > in_edges[i].v) { ok[u] = 0; return; }
        });
        return parlay::reduce(ok, parlay::logical_and<uint8_t>());
    }

    bool equal(const Graph &o) const {
        if (n != o.n || m != o.m) return false;
        if (symmetrized != o.symmetrized || weighted != o.weighted) return false;
        if (load_time != o.load_time) return false;
        if (name.size() != o.name.size()) return false;
        if (offsets.size() != o.offsets.size() || edges.size() != o.edges.size()) return false;
        if (in_offsets_seq.size() != o.in_offsets_seq.size() || in_edges_seq.size() != o.in_edges_seq.size()) return false;
        if (std::memcmp(name.data(), o.name.data(), name.size())) return false;
        auto cmp_bytes = [&](const void *a_, const void *b_, size_t bytes) -> bool {
            if (!bytes) return true;
            constexpr size_t B = 1 << 20;
            size_t k = (bytes + B - 1) / B;
            parlay::sequence<uint8_t> ok(k, 1);
            parlay::parallel_for(0, k, [&](size_t i) {
                size_t l = i * B, r = std::min(bytes, l + B);
                ok[i] = !std::memcmp((const char*)a_ + l, (const char*)b_ + l, r - l);
            });
            return parlay::reduce(ok, parlay::logical_and<uint8_t>());
        };
        if (!cmp_bytes(offsets.data(), o.offsets.data(), offsets.size() * sizeof(uint64_t))) return false;
        if (!cmp_bytes(edges.data(), o.edges.data(), edges.size() * sizeof(Edge<Wgh>))) return false;
        if (!cmp_bytes(in_offsets_seq.data(), o.in_offsets_seq.data(), in_offsets_seq.size() * sizeof(uint64_t))) return false;
        if (!cmp_bytes(in_edges_seq.data(), o.in_edges_seq.data(), in_edges_seq.size() * sizeof(Edge<Wgh>))) return false;
        return true;
    }
};

} // namespace GraphIO
