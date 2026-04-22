// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <limits>


namespace graph_io {

namespace internal {

static inline uint64_t mix64(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}
std::string get_hash(const parlay::sequence<uint32_t>& results) {
    uint64_t maxv = 0;
    uint64_t mixed_sum = 0;
    uint64_t hash64 = 0xcbf29ce484222325ULL;
    uint32_t hash32 = 2166136261u;
    for (size_t i = 0; i < results.size(); i++) {
        uint64_t v = results[i];
        if (v > maxv) maxv = v;
        mixed_sum += mix64(v + 0x9e3779b97f4a7c15ULL * i);
        hash64 ^= mix64(v + (i << 1));
        hash64 *= 0x100000001b3ULL;
        hash32 ^= static_cast<uint32_t>(v + i);
        hash32 *= 16777619u;
    }
    std::ostringstream oss;
    uint32_t folded =
        static_cast<uint32_t>(maxv) ^
        static_cast<uint32_t>(mixed_sum) ^
        static_cast<uint32_t>(mixed_sum >> 32) ^
        static_cast<uint32_t>(hash64) ^
        static_cast<uint32_t>(hash64 >> 32);
    oss << std::hex << std::setfill('0') << std::setw(8) << folded;
    return oss.str();
}
using CSV = std::vector<std::vector<std::string>>;
inline CSV read_csv(const std::string& path) {
    CSV table;
    std::ifstream ifs(path);
    if (!ifs.good()) return table;
    std::string line;
    while (std::getline(ifs, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        table.push_back(row);
    }
    return table;
}
inline void write_csv(const std::string& path, const CSV& table) {
    std::ofstream ofs(path);
    for (const auto& row : table) {
        for (size_t i = 0; i < row.size(); i++) {
            if (i) ofs << ',';
            ofs << row[i];
        }
        ofs << '\n';
    }
}
inline void update_csv_cell(const std::string& path, const std::string& graph, const std::string& column, const std::string& value) {
    CSV table = read_csv(path);
    if (table.empty()) {
        table.push_back({"graph", column});
    }
    auto& header = table[0];
    size_t col = 0;
    for (; col < header.size(); col++) {
        if (header[col] == column) break;
    }
    if (col == header.size()) {
        header.push_back(column);
    }
    for (size_t i = 1; i < table.size(); i++) {
        if (table[i].size() < header.size()) {
            table[i].resize(header.size(), "");
        }
    }
    size_t row = 1;
    for (; row < table.size(); row++) {
        if (!table[row].empty() && table[row][0] == graph) break;
    }
    if (row == table.size()) {
        table.push_back(std::vector<std::string>(header.size(), ""));
        table[row][0] = graph;
    }
    table[row][col] = value;
    write_csv(path, table);
}

} // namespace internal

std::string extract_graph_name(std::string path) {
    std::string name = std::string(path).substr(0, std::string(path).find_last_of('.'));
    if (name.find_last_of('/') != std::string::npos) { name = name.substr(name.find_last_of('/') + 1); }
    return name;
}

template<class T> bool availability(parlay::sequence<T>& result, double proportion){
    size_t n=result.size(); if(!n)return proportion<=0;
    size_t bs=1<<15, m=(n+bs-1)/bs;
    auto cnt=parlay::tabulate(m,[&](size_t b){
        size_t l=b*bs, r=std::min(n,l+bs),s=0;
        for(size_t i=l;i<r;++i)s += (result[i]!=std::numeric_limits<T>::max() && result[i]!=UINT32_MAX && result[i]!=INT32_MAX);
        return s;
    });
    size_t tot = parlay::reduce(cnt);
    return (double)tot/n >= proportion;
}

template <class T>
void process_result(double t, T& result, std::string csvpath, std::string row, std::string column, std::string dumppath = "") {
    parlay::sequence<uint32_t> res_32(result.size(), 0);
    parlay::parallel_for(0, result.size(), [&](size_t i){ res_32[i] = result[i]; });
    std::cout << "Running Time: " << t << " sec\n";
    using result_type = typename T::value_type;
    auto max_inputs = parlay::delayed_tabulate(result.size(), [&](size_t i) -> result_type {
        result_type v = result[i];
        return v == std::numeric_limits<result_type>::max() ? std::numeric_limits<result_type>::min() : v;
    });
    result_type result_max = parlay::reduce(max_inputs, parlay::maxm<result_type>());
    std::cout << "Maximum Res : " << result_max << '\n';
    std::string res_hash = internal::get_hash(res_32);
    std::cout << "Verify  Hash: " << res_hash << '\n';
    internal::update_csv_cell(csvpath + "/benchmark.csv", row, column, std::to_string(t));
    internal::update_csv_cell(csvpath + "/verify.csv",    row, column, res_hash);
    if (dumppath != "disabled" && dumppath != "") {
        std::ofstream out(dumppath, std::ios::out|std::ios::trunc);
        for(size_t i=0;i<result.size();++i){ out<<result[i]<<","; }
        out<<"\n";
    }
}

} // namespace GraphIO
