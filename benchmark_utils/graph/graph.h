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
        if (!symmetrized) { make_inverse(); }
        if constexpr (std::is_same_v<Wgh, int32_t>) { if (!weighted) make_random_weight(l, r); }
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
        auto chars = parlay::chars_from_file(std::string(filename));
        auto tokens_seq = tokens(chars);
        auto header = tokens_seq[0];
        n = chars_to_ulong_long(tokens_seq[1]);
        m = chars_to_ulong_long(tokens_seq[2]);
        if (n == 0) { std::cerr << "Error: Failed in reading graph.\n"; abort(); }
        bool weighted_input;
        if (header == parlay::to_chars("WeightedAdjacencyGraph")) { weighted_input = true; assert(tokens_seq.size() == n + m + m + 3); } 
        else if (header == parlay::to_chars("AdjacencyGraph")) { weighted_input = false; assert(tokens_seq.size() == n + m + 3); } 
        else { std::cerr << "Unrecognized PBBS format\n"; abort(); }
        offsets = parlay::sequence<uint64_t>(n + 1);
        edges = parlay::sequence<Edge<Wgh>>(m);
        parlay::parallel_for(0, n, [&](size_t i) { offsets[i] = parlay::internal::chars_to_int_t<uint64_t>( make_slice(tokens_seq[i + 3])); });
        offsets[n] = m;
        parlay::parallel_for(0, m, [&](size_t i) {
            edges[i].v = parlay::internal::chars_to_int_t<uint32_t>(make_slice(tokens_seq[i + n + 3]));
        });
        if (weighted_input) {
            if constexpr (std::is_same_v<Wgh, Empty>) { std::cout << "Warning: skipping edge weights in file\n"; } 
            else {
                weighted = true;
                parlay::parallel_for(0, m, [&](size_t i) {
                    edges[i].w = parlay::internal::chars_to_int_t<Wgh>(make_slice(tokens_seq[i + n + m + 3]));
                });
            }
        }
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

};
