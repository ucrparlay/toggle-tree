#pragma once

#include <toggle/toggle.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <limits>

#include "sampler.h"

template <class Graph>
parlay::sequence<uint32_t> KCore(Graph& G) {
  using NodeId = uint32_t;

  static constexpr bool enable_sampling = true;
  static constexpr uint32_t sample_threshold = 2000;
  static constexpr double init_reduce_ratio = 0.1;
  static constexpr uint32_t log2_error_factor = 32;
  static constexpr double bias_factor = 0.5;
  static constexpr double error_rate_tolerance = 0.0000000001;
  static constexpr uint32_t exp_hits =
      log2_error_factor / (init_reduce_ratio * init_reduce_ratio);
  static constexpr uint32_t min_threshold_sample_degree =
      static_cast<uint32_t>(sample_threshold / init_reduce_ratio + 0.999999999);
  static constexpr uint32_t min_exp_sample_degree =
      static_cast<uint32_t>(exp_hits / (1 - init_reduce_ratio)) + 1;
  static constexpr uint32_t min_initial_sample_degree =
      std::max(min_threshold_sample_degree, min_exp_sample_degree);

  const size_t n = G.n;
  auto active = toggle::Active(n);
  auto frontier = toggle::Frontier(n);

  auto coreness = parlay::sequence<NodeId>::uninitialized(n);
  auto result = parlay::sequence<NodeId>(n, 0);

  auto can_sample_initially = [&](NodeId deg) {
    return deg >= min_initial_sample_degree;
  };

  auto run_plain_kcore = [&]() {
    while (!active.empty()) {
      NodeId k = active.reduce_min(coreness);

      active.for_each([&](NodeId u) {
        if (coreness[u] == k) {
          active.remove(u);
          frontier.insert_next(u);
        }
      });

      while (frontier.advance_to_next()) {
        frontier.for_each([&](NodeId u) {
          result[u] = k;
          parlay::parallel_for(G.offsets[u], G.offsets[u + 1], [&](size_t i) {
            NodeId v = G.edges[i].idx;
            if (active.contains(v) &&
                __atomic_fetch_sub(&coreness[v], 1, __ATOMIC_RELAXED) ==
                    k + 1) {
              active.remove(v);
              frontier.insert_next(v);
            }
          }, 256);
        });
      }
    }
  };

  if (!enable_sampling || G.m < min_initial_sample_degree) {
    parlay::parallel_for(0, n, [&](size_t i) {
      coreness[i] = G.offsets[i + 1] - G.offsets[i];
      if (coreness[i] == 0) {
        active.remove(i);
      }
    });
    run_plain_kcore();
    return result;
  }

  std::atomic<bool> contains_sampling_nodes(false);
  parlay::parallel_for(0, n, [&](size_t i) {
    coreness[i] = G.offsets[i + 1] - G.offsets[i];
    if (coreness[i] == 0) {
      active.remove(i);
    }
    if (enable_sampling && can_sample_initially(coreness[i])) {
      contains_sampling_nodes.store(true, std::memory_order_relaxed);
    }
  });

  if (!contains_sampling_nodes.load(std::memory_order_relaxed)) {
    run_plain_kcore();
    return result;
  }

  auto counting_set = toggle::Active(n, false);
  auto sampling_set = toggle::Active(n, false);
  auto sample_mode = parlay::sequence<bool>(n, false);
  auto samplers = parlay::sequence<Sampler>::uninitialized(n);

  auto set_sampler = [&](NodeId v, NodeId k) {
    NodeId deg = coreness[v];
    if (deg * init_reduce_ratio >= sample_threshold &&
        k < deg * init_reduce_ratio * bias_factor &&
        exp_hits < (deg - k) * (1 - init_reduce_ratio)) {
      if (!sample_mode[v]) {
        sampling_set.insert(v);
      }
      sample_mode[v] = true;
      int n_star = static_cast<int>(deg - k);
      uint32_t log_n_star = parlay::log2_up(n_star);
      size_t modified_exp_hits =
          log2_error_factor * log_n_star * log_n_star / 2;
      double sample_rate = static_cast<double>(modified_exp_hits) /
                           ((1 - init_reduce_ratio) * deg);
      samplers[v].reset(modified_exp_hits, sample_rate);
    } else {
      if (sample_mode[v]) {
        sampling_set.remove(v);
      }
      sample_mode[v] = false;
    }
  };

  const double log_error_rate_tolerance = std::log(error_rate_tolerance);
  auto sample_needs_count = [&](NodeId v, NodeId k) -> bool {
    if (coreness[v] * init_reduce_ratio * bias_factor < k) {
      return true;
    }
    double n_star = static_cast<double>(coreness[v] - k);
    size_t num_hits = std::max((uint32_t)1, samplers[v].get_num_hits());
    double sample_rate = static_cast<double>(samplers[v].get_exp_hits()) /
                         ((1 - init_reduce_ratio) * coreness[v]);
    double mu = n_star * sample_rate;
    double h = static_cast<double>(num_hits);
    return -mu + 2 * h - h * h / mu >= log_error_rate_tolerance;
  };

  auto count_alive_neighbors = [&](NodeId u) {
    if (frontier.empty_next()) {
      coreness[u] =
          parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]),
                           [&](auto& es) { return active.contains(es.idx); });
    } else {
      coreness[u] =
          parlay::count_if(G.edges.cut(G.offsets[u], G.offsets[u + 1]),
                           [&](auto& es) {
                             return active.contains(es.idx) ||
                                    frontier.contains_next(es.idx);
                           });
    }
  };

  auto count_vertex = [&](NodeId u, NodeId k) {
    if (!active.contains(u)) {
      if (sample_mode[u]) {
        sampling_set.remove(u);
      }
      sample_mode[u] = false;
      return;
    }

    count_alive_neighbors(u);

    bool still_active = true;
    if (coreness[u] < k) {
      NodeId alive_last_round = parlay::count_if(
          G.edges.cut(G.offsets[u], G.offsets[u + 1]), [&](auto& es) {
            return active.contains(es.idx) || coreness[es.idx] == k;
          });

      if (alive_last_round >= k) {
        coreness[u] = k;
        active.remove(u);
        if (sample_mode[u]) {
          sampling_set.remove(u);
        }
        sample_mode[u] = false;
        frontier.insert_next(u);
        still_active = false;
      } else {
        assert(coreness[u] >= k);
      }
    }

    if (still_active) {
      set_sampler(u, k);
    }
  };

  auto sample_vertex = [&](NodeId u, NodeId v) {
    uint32_t hash_v = parlay::hash32(u * G.n + v);
    bool callback = false;
    samplers[v].sample(hash_v, callback);
    if (callback) {
      counting_set.insert(v);
    }
  };

  auto map_neighbors = [&](NodeId u, NodeId k, bool has_sampling) {
    if (!has_sampling) {
      parlay::parallel_for(G.offsets[u], G.offsets[u + 1], [&](size_t i) {
        NodeId v = G.edges[i].idx;
        if (active.contains(v) &&
            __atomic_fetch_sub(&coreness[v], 1, __ATOMIC_RELAXED) == k + 1) {
          active.remove(v);
          frontier.insert_next(v);
        }
      }, 256);
    } else {
      parlay::parallel_for(G.offsets[u], G.offsets[u + 1], [&](size_t i) {
        NodeId v = G.edges[i].idx;
        if (!active.contains(v)) {
          return;
        }

        if (coreness[v] >= min_initial_sample_degree && sample_mode[v]) {
          sample_vertex(u, v);
        } else {
          if (__atomic_fetch_sub(&coreness[v], 1, __ATOMIC_RELAXED) == k + 1) {
            active.remove(v);
            frontier.insert_next(v);
          }
        }
      }, 256);
    }
  };

  parlay::parallel_for(0, n, [&](size_t i) {
    if (enable_sampling && can_sample_initially(coreness[i])) {
      set_sampler(i, 0);
    }
  });

  while (!active.empty()) {
    NodeId k = active.reduce_min(coreness);

    bool has_sampling = !sampling_set.empty();
    if (has_sampling) {
      sampling_set.for_each([&](NodeId u) {
        if (sample_needs_count(u, k)) {
          count_vertex(u, k);
        }
      });
    }

    has_sampling = !sampling_set.empty();
    active.for_each([&](NodeId u) {
      if (coreness[u] == k) {
        if (has_sampling && sample_mode[u]) {
          count_alive_neighbors(u);
          if (coreness[u] > k) {
            set_sampler(u, k);
            return;
          }
        }

        active.remove(u);
        if (has_sampling && sample_mode[u]) {
          sampling_set.remove(u);
          sample_mode[u] = false;
        }
        frontier.insert_next(u);
      }
    });

    while (frontier.advance_to_next()) {
      has_sampling = !sampling_set.empty();
      frontier.for_each([&](NodeId u) {
        result[u] = k;
        map_neighbors(u, k, has_sampling);
      });

      if (!counting_set.empty()) {
        counting_set.for_each<true>([&](NodeId u) {
          count_vertex(u, k);
        });
      }
    }
  }
  return result;
}
