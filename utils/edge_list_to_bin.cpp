#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

struct Edge {
  uint32_t u;
  uint32_t v;
};

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <input.txt> <output.bin> [--sym] [--one-based]\n";
    return 1;
  }

  const std::string input_path = argv[1];
  const std::string output_path = argv[2];
  bool symmetrize = false;
  bool one_based = false;

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--sym") {
      symmetrize = true;
    } else if (arg == "--one-based") {
      one_based = true;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      return 1;
    }
  }

  std::ifstream in(input_path);
  if (!in.is_open()) {
    std::cerr << "Error: cannot open input file " << input_path << "\n";
    return 1;
  }

  std::vector<Edge> edges;
  edges.reserve(1024);
  uint64_t max_node = 0;
  std::string line;
  size_t line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream iss(line);
    uint64_t u64 = 0;
    uint64_t v64 = 0;
    if (!(iss >> u64 >> v64)) {
      std::cerr << "Error: bad line " << line_no
                << " (expected two integers)\n";
      return 1;
    }
    if (one_based) {
      if (u64 == 0 || v64 == 0) {
        std::cerr << "Error: one-based input has zero at line " << line_no
                  << "\n";
        return 1;
      }
      --u64;
      --v64;
    }
    if (u64 > std::numeric_limits<uint32_t>::max() ||
        v64 > std::numeric_limits<uint32_t>::max()) {
      std::cerr << "Error: node id exceeds uint32_t at line " << line_no
                << "\n";
      return 1;
    }
    edges.push_back(Edge{static_cast<uint32_t>(u64),
                         static_cast<uint32_t>(v64)});
    if (u64 > max_node) {
      max_node = u64;
    }
    if (v64 > max_node) {
      max_node = v64;
    }
  }
  in.close();

  if (symmetrize) {
    const size_t original_m = edges.size();
    edges.reserve(original_m * 2);
    for (size_t i = 0; i < original_m; ++i) {
      const uint32_t u = edges[i].u;
      const uint32_t v = edges[i].v;
      if (u != v) {
        edges.push_back(Edge{v, u});
      }
    }
  }

  const uint64_t n = (edges.empty() ? 0 : (max_node + 1));
  const uint64_t m = edges.size();

  std::vector<uint64_t> degrees(n, 0);
  for (const auto& e : edges) {
    degrees[e.u]++;
  }

  std::vector<uint64_t> offsets(n + 1, 0);
  for (uint64_t i = 0; i < n; ++i) {
    offsets[i + 1] = offsets[i] + degrees[i];
  }

  std::vector<uint64_t> positions = offsets;
  std::vector<uint32_t> out_edges(m);
  for (const auto& e : edges) {
    uint64_t pos = positions[e.u]++;
    out_edges[pos] = e.v;
  }

  if (sizeof(size_t) != 8) {
    std::cerr << "Error: binary format expects 8-byte size_t\n";
    return 1;
  }

  const size_t sizes = (n + 1) * 8 + m * 4 + 3 * 8;
  std::ofstream out(output_path, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "Error: cannot open output file " << output_path << "\n";
    return 1;
  }
  out.write(reinterpret_cast<const char*>(&n), sizeof(size_t));
  out.write(reinterpret_cast<const char*>(&m), sizeof(size_t));
  out.write(reinterpret_cast<const char*>(&sizes), sizeof(size_t));
  out.write(reinterpret_cast<const char*>(offsets.data()),
            sizeof(uint64_t) * (n + 1));
  out.write(reinterpret_cast<const char*>(out_edges.data()),
            sizeof(uint32_t) * m);
  out.close();

  std::cout << "Wrote binary graph: n=" << n << " m=" << m << " to "
            << output_path << "\n";
  return 0;
}
