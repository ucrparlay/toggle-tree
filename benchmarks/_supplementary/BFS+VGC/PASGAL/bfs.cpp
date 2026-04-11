#include "bfs.h"

#include <queue>

#include "graph.h"
#include "seq-bfs.h"
#include <graph_io/graph_io.h>

constexpr int NUM_SRC = 5;
constexpr int NUM_ROUND = 3;

template <class Algo, class Graph, class NodeId = typename Graph::NodeId>
void run(Algo &algo, const Graph &G, bool verify, NodeId num_rounds, const char* input_path) {
    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    parlay::sequence<uint32_t> result; uint32_t base = 0;
    for (uint32_t i = 0; i < num_rounds + base; i++) {
        auto s = perm[i];
        t.start(); result = algo.bfs(s); tt = t.stop();
        if (!graph_io::availability(result, 0.1)) { base++; continue; }
        std::cout << "    Round " << i + 1 - base << "  source: " << s << "  Warmup: "  << std::setprecision(2) << tt << std::setprecision(6);
        t.start(); result = algo.bfs(s); tt = t.stop();
        std::cout << "  time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;
    graph_io::process_result(ttt, result, "..", graph_io::extract_graph_name(input_path), "PASGAL"); 
    printf("\n");
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr,
            "Usage: %s [-i input_file] [-s] [-v]\n"
            "Options:\n"
            "\t-i,\tinput file path\n"
            "\t-s,\tsymmetrized input graph\n"
            "\t-v,\tverify result\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }
  char c;
  char const *input_path = nullptr;
  bool symmetrized = false;
  bool verify = false;
  uint32_t source = 0;
  uint32_t num_rounds = 3;
  while ((c = getopt(argc, argv, "i:svrn:")) != -1) {
    switch (c) {
      case 'i':
        input_path = optarg;
        break;
      case 's':
        symmetrized = true;
        break;
      case 'v':
        verify = true;
        break;
      case 'r':
        source = atol(optarg);
      case 'n':
        num_rounds = atol(optarg);
    }
  }

  printf("Reading graph...\n");
  Graph G;
  G.read_graph(input_path);
  G.symmetrized = symmetrized;
  if (!G.symmetrized) {
    G.make_inverse();
  }

  fprintf(stdout, "Running on %s: |V|=%zu, |E|=%zu, num_src=%d, num_round=%d\n",
          input_path, G.n, G.m, NUM_SRC, NUM_ROUND);

  BFS solver(G);
  run(solver, G, verify, num_rounds, input_path);
  return 0;
}
