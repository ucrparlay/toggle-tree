#pragma once
#include <ParSet/ParSet.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "../../_supplementary/KCore+Sampling/utils/sampler.h"

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
  using NodeId = uint32_t;
  const size_t n = G.n;
  auto result = parlay::sequence<uint32_t>(n, 0);
  auto active = ParSet::Active(n);
  auto frontier = ParSet::Frontier(n);
  auto D = parlay::tabulate<uint32_t>(n, [&](size_t s) {
    if (G.offsets[s + 1] - G.offsets[s] == 0) {
      active.remove(s);
    }
    return G.offsets[s + 1] - G.offsets[s];
  });

  // ===== Sampling module begin =====
  static constexpr bool enable_sampling = true;
  static constexpr uint32_t sample_threshold = 2000;
  static constexpr double init_reduce_ratio = 0.1;
  static constexpr uint32_t log2_error_factor = 32;
  static constexpr double bias_factor = 0.5;
  static constexpr double error_rate_tolerance = 0.0000000001;
  static constexpr uint32_t exp_hits =
      log2_error_factor / (init_reduce_ratio * init_reduce_ratio);

  // Bitmap-based set for vertices whose samplers fired this peel round.
  auto counting_bag = ParSet::Active(n, false);
  auto sample_mode = parlay::sequence<bool>(n, false);
  auto samplers = parlay::sequence<Sampler>::uninitialized(n);

  auto set_sampler = [&](NodeId v, NodeId k) {
    if (D[v] * init_reduce_ratio >= sample_threshold &&
        k < D[v] * init_reduce_ratio * bias_factor &&
        exp_hits < (D[v] - k) * (1 - init_reduce_ratio)) {
      sample_mode[v] = true;
      size_t modified_exp_hits = log2_error_factor *
                                 parlay::log2_up((int)(D[v] - k)) *
                                 parlay::log2_up((int)(D[v] - k)) / 2;
      double sample_rate = modified_exp_hits / ((1 - init_reduce_ratio) * D[v]);
      samplers[v].reset(modified_exp_hits, sample_rate);
    } else {
      sample_mode[v] = false;
    }
  };

  auto check_sample_security = [&](NodeId v, NodeId k) -> double {
    if (D[v] * init_reduce_ratio * bias_factor < k) {
      return 1;
    }
    int n_star = D[v] - k;
    size_t num_hits = std::max((uint32_t)1, samplers[v].get_num_hits());
    double sample_rate =
        samplers[v].get_exp_hits() / ((1 - init_reduce_ratio) * D[v]);
    return std::exp(-1.0 * n_star * sample_rate + 2 * num_hits -
                    1.0 * num_hits * num_hits / n_star / sample_rate);
  };

  auto count_alive_neighbors = [&](NodeId u) {
    D[u] = parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]),
                            [&](auto& es) { return active.contains(es.v); });
  };

  auto count_vertex = [&](NodeId u, NodeId k) {
    count_alive_neighbors(u);
    if (D[u] < k) {
      D[u] = k;
      if (active.contains(u)) {
        active.remove(u);
        frontier.insert_next(u);
      }
    }
    set_sampler(u, k);
  };

  bool contains_sampling_nodes = false;
  parlay::parallel_for(0, n, [&](size_t i) {
    if (enable_sampling && D[i] * init_reduce_ratio >= sample_threshold) {
      contains_sampling_nodes = true;
    }
  });
  if (contains_sampling_nodes) {
    parlay::parallel_for(0, n, [&](size_t i) { set_sampler(i, 0); });
  }
  // ===== Sampling module end =====

  parlay::internal::timer t;
  double t0 = 0;
  while (!active.empty()) {
    uint32_t k = active.reduce_min(D);
    // Sampling: re-check vertices in sample_mode whose error rate exceeds
    // tolerance.
    if (contains_sampling_nodes) {
      active.for_each([&](NodeId u) {
        if (sample_mode[u] &&
            check_sample_security(u, k) >= error_rate_tolerance) {
          count_vertex(u, k);
        }
      });
    }
    active.for_each([&](uint32_t s) {
      if (D[s] == k) {
        active.remove(s);
        frontier.insert_next(s);
      }
    });
    while (frontier.advance_to_next()) {
      bool counting_flag = false;
      const size_t LOCAL_QUEUE = 128;
      const size_t STRIDE = 8;
      auto local_search = [&](auto&& self, uint32_t s, uint32_t layer) -> void {
        if (G.offsets[s + 1] - G.offsets[s] >= LOCAL_QUEUE) {
          result[s] = k;
          parlay::parallel_for(
              G.offsets[s], G.offsets[s + 1],
              [&](size_t i) {
                uint32_t d = G.edges[i].v;
                if (active.contains(d)) {
                  if (enable_sampling && sample_mode[d]) {
                    uint32_t hash_v = parlay::hash32(s * G.n + d);
                    bool callback = false;
                    samplers[d].sample(hash_v, callback);
                    if (callback) {
                      counting_flag = true;
                      counting_bag.insert(d);
                    }
                  } else if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) ==
                             k + 1) {
                    active.remove(d);
                    if (layer + 1 == STRIDE)
                      frontier.insert_next(d);
                    else
                      self(self, d, layer + 1);
                  }
                }
              },
              LOCAL_QUEUE);
        } else {
          uint32_t local_queue[LOCAL_QUEUE];
          local_queue[0] = s;
          size_t lpos = 0, rpos = 1, cnt = 1;
          while (lpos < rpos && cnt < LOCAL_QUEUE) {
            uint32_t cur_index = local_queue[lpos];
            result[cur_index] = k;
            if (G.offsets[cur_index + 1] - G.offsets[cur_index] + rpos >
                LOCAL_QUEUE) {
              break;
            }
            lpos++;
            for (uint32_t i = G.offsets[cur_index];
                 i < G.offsets[cur_index + 1]; i++) {
              cnt++;
              uint32_t d = G.edges[i].v;
              if (active.contains(d)) {
                if (enable_sampling && sample_mode[d]) {
                  uint32_t hash_v = parlay::hash32(cur_index * G.n + d);
                  bool callback = false;
                  samplers[d].sample(hash_v, callback);
                  if (callback) {
                    counting_flag = true;
                    counting_bag.insert(d);
                  }
                } else if (__atomic_fetch_sub(&D[d], 1, __ATOMIC_RELAXED) ==
                           k + 1) {
                  active.remove(d);
                  local_queue[rpos] = d;
                  rpos++;
                }
              }
            }
          }
          parlay::parallel_for(lpos, rpos, [&](uint32_t i) {
            if (layer + 1 == STRIDE)
              frontier.insert_next(local_queue[i]);
            else
              self(self, local_queue[i], layer + 1);
          });
        }
      };
      frontier.for_each([&](uint32_t s) { local_search(local_search, s, 0); });
      // Sampling: process vertices whose samplers fired during this peel round.
      if (counting_flag) {
        counting_bag.for_each<true>(
            [&](NodeId u) { count_vertex(u, k); });
      }
    }
  }

  return result;
}
