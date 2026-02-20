#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "include/graph.h"

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::fprintf(stderr,
                 "Usage: %s -i <input_graph> -t <threshold> [-s]\n"
                 "Options:\n"
                 "\t-i,\tinput file path\n"
                 "\t-t,\tdegree threshold (count degree > threshold)\n"
                 "\t-s,\tinput graph already symmetrized\n",
                 argv[0]);
    return 1;
  }

  const char *input_path = nullptr;
  bool symmetrized = false;
  unsigned long long threshold = 0;
  bool threshold_set = false;
  int c;
  while ((c = getopt(argc, argv, "i:t:s")) != -1) {
    switch (c) {
      case 'i':
        input_path = optarg;
        break;
      case 't':
        threshold = std::strtoull(optarg, nullptr, 10);
        threshold_set = true;
        break;
      case 's':
        symmetrized = true;
        break;
      default:
        return 1;
    }
  }

  if (input_path == nullptr || !threshold_set) {
    std::fprintf(stderr, "Error: input path and threshold are required.\n");
    return 1;
  }

  std::printf("Reading graph...\n");
  Graph G;
  G.read_graph(input_path);
  if (!symmetrized) {
    G = make_symmetrized(G);
  }
  G.symmetrized = true;

  size_t count = 0;
  for (size_t u = 0; u < G.n; ++u) {
    const size_t degree = G.offsets[u + 1] - G.offsets[u];
    if (degree > threshold) {
      ++count;
    }
  }

  std::printf("Vertices with degree > %" PRIu64 ": %zu\n",
              static_cast<uint64_t>(threshold), count);
  return 0;
}
