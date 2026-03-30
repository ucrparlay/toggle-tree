#include "bfs.h"

#include <queue>

#include "graph.h"
#include "seq-bfs.h"
#include "verify.h"

constexpr int NUM_SRC = 5;
constexpr int NUM_ROUND = 3;

template <class Algo, class Graph, class NodeId = typename Graph::NodeId>
void run(Algo &algo, const Graph &G, bool verify, NodeId num_rounds, const char* input_path) {
  //printf("source %-10d\n", s);
  auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    t.start();
    parlay::sequence<uint32_t> dist;
    for (int i = 0; i < (int)num_rounds; i++) {
        auto s = perm[num_rounds - i - 1];
        std::cout << "Round " << i + 1 << "  source = " << s;
        t.start();
        parlay::execute_with_scheduler(1, [&] { dist = algo.bfs(s);});
        std::cout << "  Warmup = " << t.stop();
        t.start();
        parlay::execute_with_scheduler(1, [&] { dist = algo.bfs(s);});
        tt = t.stop();
        std::cout << " time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;
    process_result(nullptr, input_path, ttt, dist, true);  
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
