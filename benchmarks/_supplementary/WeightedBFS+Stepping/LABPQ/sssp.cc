#include "sssp.h"

#include <fstream>
#include <functional>
#include <numeric>
#include <iostream>

#include "dijkstra.h"

#include <GraphIO/GraphIO.h>

using namespace std;
using namespace parlay;

template <class Algo>
void run(Algo &algo, const Graph &G, bool verify, const char* filepath) {
    std::cout << "==================================================================" << std::endl;
    std::cout << "### Graph: " << GraphIO::extract_graph_name(filepath) << "\n";
    std::cout << "### Threads: " << parlay::num_workers() << "\n";

    int num_rounds = 5;
    auto perm = parlay::random_permutation<uint32_t>(G.n);
    parlay::internal::timer t; double tt = 0, ttt = 0;
    parlay::sequence<uint32_t> result; uint32_t base = 0;
    for (uint32_t i = 0; i < num_rounds + base; i++) {
        auto s = perm[i];
        t.start(); result = algo.sssp(s); tt = t.stop();
        if (!GraphIO::availability(result, 0.1)) { base++; continue; }
        std::cout << "    Round " << i + 1 - base << "  source: " << s << "  Warmup: "  << std::setprecision(2) << tt << std::setprecision(6);
        t.start(); result = algo.sssp(s); tt = t.stop();
        std::cout << "  time = " << tt << " sec\n";
        ttt += tt;
    }
    ttt /= num_rounds;

    if (verify) {
        GraphIO::process_result("disabled", filepath, ttt, result, true);  
    }
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(
        stderr,
        "Usage: %s [-i input_file] [-p parameter] [-w] [-s] [-v] [-a "
        "algorithm]\n"
        "Options:\n"
        "\t-i,\tinput file path\n"
        "\t-p,\tparameter(e.g. delta, rho)\n"
        "\t-w,\tweighted input graph\n"
        "\t-s,\tsymmetrized input graph\n"
        "\t-v,\tverify result\n"
        "\t-a,\talgorithm: [rho-stepping] [delta-stepping] [bellman-ford]\n",
        argv[0]);
    exit(EXIT_FAILURE);
  }
  char c;
  bool weighted = false;
  bool symmetrized = false;
  bool verify = false;
  string param;
  int algo = rho_stepping;
  char const *FILEPATH = nullptr;
  while ((c = getopt(argc, argv, "i:p:a:wsv")) != -1) {
    switch (c) {
      case 'i':
        FILEPATH = optarg;
        break;
      case 'p':
        param = string(optarg);
        break;
      case 'a':
        if (!strcmp(optarg, "rho-stepping")) {
          algo = rho_stepping;
        } else if (!strcmp(optarg, "delta-stepping")) {
          algo = delta_stepping;
        } else if (!strcmp(optarg, "bellman-ford")) {
          algo = bellman_ford;
        } else {
          fprintf(stderr, "Error: Unknown algorithm %s\n", optarg);
          exit(EXIT_FAILURE);
        }
        break;
      case 'w':
        weighted = true;
        break;
      case 's':
        symmetrized = true;
        break;
      case 'v':
        verify = true;
        break;
      default:
        fprintf(stderr, "Error: Unknown option %c\n", optopt);
        exit(EXIT_FAILURE);
    }
  }
  Graph G(weighted, symmetrized);

  //printf("Reading graph...\n");
  G.read_graph(FILEPATH);
  if (!weighted) {
    printf("Generating edge weights...\n");
    G.generate_weight();
  }

  //fprintf(stdout,
  //        "Running on %s: |V|=%zu, |E|=%zu, param=%s, num_src=%d, "
  //        "num_round=%d\n",
  //        FILEPATH, G.n, G.m, param.c_str(), NUM_SRC, NUM_ROUND);

  int sd_scale = G.m / G.n;
  if (algo == rho_stepping) {
    size_t rho = 1 << 20;
    if (param != "") {
      rho = stoull(param);
    }
    Rho_Stepping solver(G, rho);
    solver.set_sd_scale(sd_scale);
    run(solver, G, verify, FILEPATH);
  } else if (algo == delta_stepping) {
    EdgeTy delta = 1 << 15;
    if (param != "") {
      if constexpr (is_integral_v<EdgeTy>) {
        delta = stoull(param);
      } else {
        delta = stod(param);
      }
    }
    Delta_Stepping solver(G, delta);
    solver.set_sd_scale(sd_scale);
    run(solver, G, verify, FILEPATH);
  } else if (algo == bellman_ford) {
    Bellman_Ford solver(G);
    solver.set_sd_scale(sd_scale);
    run(solver, G, verify, FILEPATH);
  }
  return 0;
}
