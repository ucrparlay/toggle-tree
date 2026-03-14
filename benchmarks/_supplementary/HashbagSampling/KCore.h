#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <utility>

#include <ParSet/ParSet.h>
#include "hashbag.h"
#include "parlay/primitives.h"
#include "sampler.h"
#include "utils.h"

template <typename E, typename EV>
inline std::pair<E, bool> fetch_and_add_bounded_hashbag(E* a, EV b, E k) {
  volatile E newV, oldV;
  bool c = false;
  do {
    oldV = *a;
    newV = oldV + b;
  } while (oldV > k && !(c = atomic_compare_and_swap(a, oldV, newV)));
  return std::make_pair(oldV, c);
}

template <class Graph>
class KCoreSampling {
  using NodeId = typename Graph::NodeId;

  static constexpr bool enable_sampling = true;
  static constexpr uint32_t sample_threshold = 2000;
  static constexpr double init_reduce_ratio = 0.1;
  static constexpr uint32_t log2_error_factor = 32;
  static constexpr double bias_factor = 0.5;
  static constexpr double error_rate_tolerance = 0.0000000001;
  static constexpr uint32_t exp_hits =
      log2_error_factor / (init_reduce_ratio * init_reduce_ratio);

  Graph& G;
  hashbag<NodeId> frontier;
  hashbag<NodeId> counting_bag;
  parlay::sequence<NodeId> frontier_buffer;
  parlay::sequence<NodeId> remaining_vertices;
  parlay::sequence<NodeId> coreness;
  parlay::sequence<NodeId> result;
  parlay::sequence<bool> alive;
  parlay::sequence<bool> sample_mode;
  parlay::sequence<Sampler> samplers;

 public:
  KCoreSampling(Graph& G) : G(G), frontier(G.n), counting_bag(G.n) {
    frontier_buffer = parlay::sequence<NodeId>::uninitialized(G.n);
    remaining_vertices = parlay::sequence<NodeId>::uninitialized(G.n);
    coreness = parlay::sequence<NodeId>::uninitialized(G.n);
    result = parlay::sequence<NodeId>(G.n, 0);
    alive = parlay::sequence<bool>::uninitialized(G.n);
    sample_mode = parlay::sequence<bool>::uninitialized(G.n);
    samplers = parlay::sequence<Sampler>::uninitialized(G.n);
  }

  void set_sampler(NodeId v, NodeId k) {
    if (coreness[v] * init_reduce_ratio >= sample_threshold &&
        k < coreness[v] * init_reduce_ratio * bias_factor &&
        exp_hits < (coreness[v] - k) * (1 - init_reduce_ratio)) {
      sample_mode[v] = true;
      size_t modified_exp_hits = log2_error_factor *
                                 parlay::log2_up((int)(coreness[v] - k)) *
                                 parlay::log2_up((int)(coreness[v] - k)) / 2;
      double sample_rate =
          modified_exp_hits / ((1 - init_reduce_ratio) * coreness[v]);
      samplers[v].reset(modified_exp_hits, sample_rate);
    } else {
      sample_mode[v] = false;
    }
  }

  double check_sample_security(NodeId v, NodeId k) {
    if (coreness[v] * init_reduce_ratio * bias_factor < k) {
      return 1;
    }
    int n_star = coreness[v] - k;
    size_t num_hits = std::max((uint32_t)1, samplers[v].get_num_hits());
    double sample_rate =
        samplers[v].get_exp_hits() / ((1 - init_reduce_ratio) * coreness[v]);
    return std::exp(-1.0 * n_star * sample_rate + 2 * num_hits -
                    1.0 * num_hits * num_hits / n_star / sample_rate);
  }

  void count_alive_neighbors(NodeId u) {
    coreness[u] = parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]),
                                   [&](auto& es) { return alive[es.v]; });
  }

  void count_vertex(NodeId u, NodeId k) {
    NodeId was = coreness[u];
    count_alive_neighbors(u);
    if (coreness[u] < k) {
      NodeId alive_last_round = parlay::count_if(
          G.edges.cut(G.offsets[u], G.offsets[u + 1]),
          [&](auto& es) { return alive[es.v] || coreness[es.v] == k; });
      if (alive_last_round >= k) {
        coreness[u] = k;
        if (alive[u]) {
          alive[u] = false;
          frontier.insert(u);
        }
      } else {
        printf("coreness[%u]: %u, was: %u, k: %u, alive_last_round: %u\n", u,
               coreness[u], was, k, alive_last_round);
        assert(coreness[u] >= k);
      }
    }
    set_sampler(u, k);
  }

  void sample_vertex(NodeId u, NodeId v, bool& counting_flag) {
    uint32_t hash_v = parlay::hash32(u * G.n + v);
    bool callback = false;
    samplers[v].sample(hash_v, callback);
    if (callback) {
      counting_flag = true;
      counting_bag.insert(v);
    }
  }

  void map_neighbors(NodeId u, NodeId k, bool& counting_flag) {
    ParSet::adaptive_for(G.offsets[u], G.offsets[u + 1], [&](size_t i) {
      NodeId v = G.edges[i].v;
      if (!alive[v]) {
        return;
      }
      if (enable_sampling && sample_mode[v]) {
        sample_vertex(u, v, counting_flag);
      } else if (__atomic_fetch_sub(&coreness[v], 1, __ATOMIC_RELAXED) == k + 1) {
        alive[v] = false;
        frontier.insert(v);
      }
    });
  }

  parlay::sequence<NodeId> run() {
    bool contains_sampling_nodes = false;
    parlay::parallel_for(0, G.n, [&](size_t i) {
      remaining_vertices[i] = i;
      coreness[i] = G.offsets[i + 1] - G.offsets[i];
      alive[i] = coreness[i] > 0;
      if (enable_sampling &&
          coreness[i] * init_reduce_ratio >= sample_threshold) {
        contains_sampling_nodes = true;
      }
    });

    if (contains_sampling_nodes) {
      parlay::parallel_for(0, G.n, [&](size_t i) { set_sampler(i, 0); });
    }

    while (!remaining_vertices.empty()) {
      NodeId k = parlay::reduce(parlay::delayed_seq<NodeId>(
          remaining_vertices.size(),
          [&](size_t i) { return coreness[remaining_vertices[i]]; }),
          parlay::minm<NodeId>());
      if (contains_sampling_nodes) {
        parlay::parallel_for(0, remaining_vertices.size(), [&](size_t i) {
          NodeId u = remaining_vertices[i];
          if (sample_mode[u] &&
              check_sample_security(u, k) >= error_rate_tolerance) {
            count_vertex(u, k);
          }
        });
      }
      parlay::parallel_for(0, remaining_vertices.size(), [&](size_t i) {
        NodeId u = remaining_vertices[i];
        if (coreness[u] == k) {
          alive[u] = false;
          frontier.insert(u);
        }
      });

      while (true) {
        size_t frontier_size = frontier.pack_into(parlay::make_slice(frontier_buffer));
        if (!frontier_size) {
          break;
        }
        bool counting_flag = false;
        parlay::parallel_for(0, frontier_size, [&](size_t i) {
          NodeId u = frontier_buffer[i];
          result[u] = k;
          map_neighbors(u, k, counting_flag);
        });
        if (counting_flag) {
          auto counting_vertices = counting_bag.pack();
          parlay::parallel_for(0, counting_vertices.size(), [&](size_t i) {
            count_vertex(counting_vertices[i], k);
          });
        }
      }
      remaining_vertices = parlay::filter(
          remaining_vertices, [&](NodeId u) { return alive[u]; });
    }

    return result;
  }
};

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
  return KCoreSampling<Graph>(G).run();
}
