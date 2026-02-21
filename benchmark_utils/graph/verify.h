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
static inline uint64_t mix64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}
std::string GetHash(const parlay::sequence<uint32_t>& results) {
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
    /*
    oss << std::hex << std::setfill('0')
        << std::setw(8) << maxv
        << std::setw(16) << mixed_sum
        << std::setw(16) << hash64;*/
    uint32_t folded =
        static_cast<uint32_t>(maxv) ^
        static_cast<uint32_t>(mixed_sum) ^
        static_cast<uint32_t>(mixed_sum >> 32) ^
        static_cast<uint32_t>(hash64) ^
        static_cast<uint32_t>(hash64 >> 32);
    oss << std::hex << std::setfill('0') << std::setw(8) << folded;
    return oss.str();
}

std::string extract_graph_name(const char* path) {
    std::string s(path);
    size_t pos = s.find_last_of('/');
    if (pos != std::string::npos) {
        s = s.substr(pos + 1);
    }
    const std::string suffix = ".bin";
    if (s.size() >= suffix.size() &&
        s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
        s.resize(s.size() - suffix.size());
    }
    return s;
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
inline void update_csv_cell(
    const std::string& path,
    const std::string& graph,
    const std::string& value,
    const std::string colu = ""
) {
    std::string column;
    if (colu == "") {
        std::string file = std::filesystem::current_path();
        column = file.substr(file.find_last_of("/\\") + 1);
    }
    else { column = colu; }
    

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
        if (!table[row].empty() && table[row][0] == graph)
            break;
    }
    if (row == table.size()) {
        table.push_back(std::vector<std::string>(header.size(), ""));
        table[row][0] = graph;
    }
    table[row][col] = value;
    write_csv(path, table);
}

template <class T>
void process_result(const char* filepath, double t, T& result, bool print = false, std::string gbbs_path = "") {
    std::string graph_name = extract_graph_name(filepath);

    if (print) std::cout << "### Running Time: " << t << " sec\n";

    parlay::sequence<uint32_t> res_32(result.size(), 0);
    for (size_t i=0;i<result.size();i++) res_32[i] = result[i];

    uint32_t maxi = 0;
    for (uint32_t i=0; i<res_32.size(); i++) if (res_32[i] != UINT32_MAX && res_32[i] > maxi) maxi = res_32[i];
    if (print) std::cout << "### Maximum Res : " << maxi << "\n";

    std::string get_hash = GetHash(res_32);
    if (print) std::cout << "### Verify  Hash: " << get_hash << '\n';

    if (gbbs_path == ""){
        update_csv_cell("../max.csv",       graph_name, std::to_string(maxi));
        update_csv_cell("../benchmark.csv", graph_name, std::to_string(t));
        update_csv_cell("../verify.csv",    graph_name, get_hash);
    }
    else {
        update_csv_cell(gbbs_path + "/max.csv",       graph_name, std::to_string(maxi), "GBBS");
        update_csv_cell(gbbs_path + "/benchmark.csv", graph_name, std::to_string(t), "GBBS");
        update_csv_cell(gbbs_path + "/verify.csv",    graph_name, get_hash, "GBBS");
    }
}