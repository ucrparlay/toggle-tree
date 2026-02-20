#pragma once
#define MODE_THRESHOLD 131072

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

#if !defined(USE_1_BUCKETS) && !defined(USE_2_BUCKETS) && \
    !defined(USE_4_BUCKETS) && !defined(USE_8_BUCKETS) && \
    !defined(USE_16_BUCKETS)
#define USE_8_BUCKETS
#endif

#if defined(USE_16_BUCKETS)
#define NUM_BUCKETS 16
#define LUT_BLOK 4
#define LUT_MASK 15
#define LUT_TYPE uint64_t
#define LUT_INIT 0xFEDCBA9876543210ull
#define LUT_MAXI 0xFFFFFFFFFFFFFFFFull
#define LUT_SIZE 64
#elif defined(USE_8_BUCKETS)
#define NUM_BUCKETS 8
#define LUT_BLOK 3
#define LUT_MASK 7
#define LUT_TYPE uint32_t
#define LUT_INIT 0b111'110'101'100'011'010'001'000u
#define LUT_MAXI 0xFFFFFFFFu
#define LUT_SIZE 24
#elif defined(USE_4_BUCKETS)
#define NUM_BUCKETS 4
#define LUT_BLOK 2
#define LUT_MASK 3
#define LUT_TYPE uint8_t
#define LUT_INIT 0b11'10'01'00u
#define LUT_MAXI 0xFFu
#define LUT_SIZE 8
#elif defined(USE_2_BUCKETS)
#define NUM_BUCKETS 2
#define LUT_BLOK 1
#define LUT_MASK 1
#define LUT_TYPE uint8_t
#define LUT_INIT 0b1'0u
#define LUT_MAXI 0xFFu
#define LUT_SIZE 2
#elif defined(USE_1_BUCKETS)
#define NUM_BUCKETS 1
#define LUT_BLOK 1
#define LUT_MASK 0
#define LUT_TYPE uint8_t
#define LUT_INIT 0b0u
#define LUT_MAXI 0xFFu
#define LUT_SIZE 1
#endif

#ifdef USE_LIBNUMA
#include <numa.h>
#include <sched.h>
#endif

template <typename E>
inline int fas(E* a) {
  volatile E oldV, newV;
  do {
    oldV = *a;
    if (oldV - 1 < 0) return oldV;
    newV = oldV - 1;
  } while (!atomic_compare_and_swap(a, oldV, newV));
  return oldV;
}

static thread_local LUT_TYPE worker_offset = LUT_MAXI;
struct alignas(64) IDs {
  LUT_TYPE next_id;
};

inline LUT_TYPE get_offset() {
  if (worker_offset != LUT_MAXI) return worker_offset;

#ifndef USE_LIBNUMA
  static LUT_TYPE next_id = 0;
  LUT_TYPE id = __atomic_fetch_add(&next_id, 1, __ATOMIC_RELAXED);
  return worker_offset = (id & LUT_MASK) * LUT_BLOK;
#else
  static int n_numa = -1;
  if (n_numa == -1) {
    if (numa_available() < 0)
      n_numa = 1;
    else
      n_numa = numa_num_configured_nodes();
  }

  int my_numa = 0;
  int cpu = sched_getcpu();
  if (cpu >= 0 && numa_available() >= 0) my_numa = numa_node_of_cpu(cpu);

  if (n_numa >= NUM_BUCKETS) {
    LUT_TYPE id = static_cast<LUT_TYPE>(my_numa % NUM_BUCKETS);
    return worker_offset = (id & LUT_MASK) * LUT_BLOK;
  }

  static IDs* ids = nullptr;
  if (ids == nullptr) {
    ids = static_cast<IDs*>(
        std::malloc(sizeof(IDs) * static_cast<size_t>(n_numa)));
    for (int i = 0; i < n_numa; ++i) ids[i].next_id = 0;
  }

  LUT_TYPE sub_id =
      __atomic_fetch_add(&ids[my_numa].next_id, 1, __ATOMIC_RELAXED);
  LUT_TYPE start = (NUM_BUCKETS * static_cast<LUT_TYPE>(my_numa)) /
                   static_cast<LUT_TYPE>(n_numa);
  LUT_TYPE end = (NUM_BUCKETS * static_cast<LUT_TYPE>(my_numa + 1)) /
                 static_cast<LUT_TYPE>(n_numa);
  LUT_TYPE span = end - start;
  LUT_TYPE id = start + (sub_id % span);
  return worker_offset = (id & LUT_MASK) * LUT_BLOK;
#endif
}

inline int num_buckets() { return NUM_BUCKETS; }
inline LUT_TYPE initial_bucket_index() {
  LUT_TYPE off = get_offset();
  return (static_cast<LUT_TYPE>(LUT_INIT) >> off) & LUT_MASK;
}

struct alignas(64) LUT {
  LUT_TYPE table;
  bool is_zero;
};
struct alignas(64) Bucket {
  int value;
};

struct alignas(64) ZeroCounter {
  LUT lookup;
  Bucket bucket[NUM_BUCKETS];

  explicit ZeroCounter(int init_count) {
    lookup.table = LUT_INIT;
    lookup.is_zero = (init_count == 0);
    int base = init_count / NUM_BUCKETS;
    int rem = init_count % NUM_BUCKETS;
    for (int i = 0; i < NUM_BUCKETS; ++i)
      bucket[i].value = base + (i < rem ? 1 : 0);
  }

  inline bool is_zero() const noexcept {
    return __atomic_load_n(&lookup.is_zero, __ATOMIC_RELAXED);
  }

  // Returns true iff this call flips is_zero from false -> true (first to set).
  inline bool set_zero() noexcept {
    return !__atomic_exchange_n(&lookup.is_zero, true, __ATOMIC_RELAXED);
  }

  inline bool decrement() noexcept {
    LUT_TYPE tbl = __atomic_load_n(&lookup.table, __ATOMIC_RELAXED);
    LUT_TYPE off = get_offset();
    LUT_TYPE idx = (tbl >> off) & LUT_MASK;

    for (int i = 0; i < NUM_BUCKETS;) {
      int old = __atomic_fetch_sub(&bucket[idx].value, 1, __ATOMIC_RELAXED);
      // int old = fas(&bucket[idx].value);

      if (old > 1) return false;
      if (old == 1) return rewrite_routing(idx);

      // CERTAIN speed win: remove (% LUT_SIZE) which can compile to division
      // for LUT_SIZE=24.
      do {
        off += LUT_BLOK;
        if (off >= LUT_SIZE) off = 0;
        i++;
      } while (((tbl >> off) & LUT_MASK) == idx && i < NUM_BUCKETS);

      idx = (tbl >> off) & LUT_MASK;
    }
    return set_zero();
  }

  inline bool seq_decrement() noexcept {
    static thread_local uint32_t rnd = 0x12345678u;
    rnd = rnd * 1664525u + 1013904223u;
    int idx = static_cast<int>(rnd & (NUM_BUCKETS - 1));

    for (int k = 0; k < NUM_BUCKETS; ++k) {
      int i = (idx + k) & (NUM_BUCKETS - 1);
      int old = bucket[i].value--;
      if (old > 1) return false;
      if (old == 1) return rewrite_routing(static_cast<LUT_TYPE>(i));
    }
    return set_zero();
  }

  inline bool seq_decrement2() noexcept {
    int max_idx = 0;
    int max_val = bucket[0].value;
    for (int i = 1; i < NUM_BUCKETS; ++i) {
      int v = bucket[i].value;
      if (v > max_val) {
        max_val = v;
        max_idx = i;
      }
    }
    if (max_val > 1) {
      bucket[max_idx].value--;
      return false;
    }
    if (max_val == 1) {
      for (int i = 0; i < NUM_BUCKETS; ++i) {
        if (i != max_idx && bucket[i].value > 0) {
          bucket[max_idx].value = 0;
          return false;
        }
      }
      bucket[max_idx].value = 0;
      return set_zero();
    }
    return false;
  }

  inline bool rewrite_routing(LUT_TYPE idx) noexcept {
    while (true) {
      int live_cnt = 0;
      LUT_TYPE live[NUM_BUCKETS];
      for (int i = 0; i < NUM_BUCKETS; ++i) {
        if (__atomic_load_n(&bucket[i].value, __ATOMIC_RELAXED) > 0)
          live[live_cnt++] = static_cast<LUT_TYPE>(i);
      }
      if (live_cnt == 0) return set_zero();

      LUT_TYPE new_tbl = 0;
      for (int i = 0; i < NUM_BUCKETS; ++i) {
        LUT_TYPE b = live[i % live_cnt];
        new_tbl |= (b << (LUT_BLOK * i));
      }

      __atomic_store_n(&lookup.table, new_tbl, __ATOMIC_RELAXED);

      // small change : avoid reload
      LUT_TYPE tbl = new_tbl;

      bool has_idx = false;
      for (LUT_TYPE off = 0; off < LUT_SIZE; off += LUT_BLOK) {
        if (((tbl >> off) & LUT_MASK) == idx) {
          has_idx = true;
          break;
        }
      }
      if (!has_idx) return false;
    }
  }

  inline uint32_t seq_load() const noexcept {
    uint32_t ret = 0;
    for (int i = 0; i < NUM_BUCKETS; ++i) {
      int v = bucket[i].value;
      if (v > 0) ret += static_cast<uint32_t>(v);
    }
    return ret;
  }

  inline void seq_store(uint32_t init_count) noexcept {
    lookup.table = LUT_INIT;
    lookup.is_zero = (init_count == 0);
    int base = static_cast<int>(init_count / NUM_BUCKETS);
    int rem = static_cast<int>(init_count % NUM_BUCKETS);
    for (int i = 0; i < NUM_BUCKETS; ++i)
      bucket[i].value = base + (i < rem ? 1 : 0);
  }
};

struct CounterPool {
  inline static ZeroCounter* pool = nullptr;
  inline static uint32_t size = 0;
  inline static uint32_t capacity = 0;

  static void init(uint32_t cap) {
    capacity = cap;
    size_t bytes = static_cast<size_t>(cap) * sizeof(ZeroCounter);
    bytes = (bytes + 63) & ~size_t(63);
    pool = static_cast<ZeroCounter*>(aligned_alloc(64, bytes));
    if (!pool) std::abort();
  }

  static inline uint32_t alloc(int init_count) {
    uint32_t idx = __atomic_fetch_add(&size, 1, __ATOMIC_RELAXED);
    new (&pool[idx]) ZeroCounter(init_count);
    return idx;
  }

  static inline ZeroCounter& get(uint32_t idx) { return pool[idx]; }
};

struct Counter {
    uint32_t data;

    Counter(uint32_t init_count) {
        if (init_count >= MODE_THRESHOLD)
        data = (CounterPool::alloc(static_cast<int>(init_count)) << 1) | 1u;
        else
        data = init_count << 1;
    }
    Counter(uint32_t init_count, bool mode) {
        if (mode) {
            data = CounterPool::alloc(static_cast<int>(init_count));
        }
        else {
            data = init_count;
        }
    }

    Counter(const Counter& other) noexcept : data(other.data) {}

    inline bool is_zero() const noexcept { return data == 0; }

    inline bool set_zero() noexcept {
        return __atomic_exchange_n(&data, 0u, __ATOMIC_ACQ_REL) != 0u;
    }

    inline bool decrement() noexcept {
        if (data == 0) return false;
        if (!(data & 1u)) {
        return __atomic_fetch_sub(&data, 2u, __ATOMIC_RELAXED) == 2u;
        }
        if (CounterPool::get(data >> 1).decrement()) {
        data = 0;
        return true;
        }
        return false;
    }

    inline bool decrement(bool mode) noexcept {
        if (mode) {
            return CounterPool::get(data).decrement();
        }
        else {
            return __atomic_fetch_sub(&data, 1, __ATOMIC_RELAXED) == 1;
        }
    }

    inline bool seq_decrement() noexcept {
        if (data == 0) return false;
        if (!(data & 1u)) {
        data -= 2u;
        return data == 0;
        }
        if (CounterPool::get(data >> 1).seq_decrement()) {
        data = 0;
        return true;
        }
        return false;
    }
    inline bool seq_decrement(bool mode) noexcept {
        if (mode) {
            return CounterPool::get(data).seq_decrement();
        }
        else {
            data -= 1;
            return data == 0;
        }
    }
};

#include "parlay/parallel.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"
struct ModeBitmap {
    parlay::sequence<uint64_t> mode;
    ModeBitmap(size_t n){ mode = parlay::sequence<uint64_t>((n + 63) / 64, 0); }
    inline bool is_mode(size_t i) { return ((mode[i >> 6] >> (i & 63)) & 1ULL); }
    inline void mark_mode(size_t i){ __atomic_fetch_or(&mode[i>>6],1ULL<<(i&63),__ATOMIC_RELAXED); }
};
struct CounterArray {
    parlay::sequence<Counter> counters;
    ModeBitmap mode;

    inline bool decrement(size_t i) { 
        return counters[i].decrement(mode.is_mode(i));
    }
    inline bool seq_decrement(size_t i) { 
        return counters[i].seq_decrement(mode.is_mode(i));
    }
    
    template <class Graph, class Insert>
    CounterArray(Graph& G, Insert&& insert): mode(G.n) { 
        CounterPool::init(G.n); 
        counters = parlay::sequence<Counter>::from_function(G.n, [&](size_t i) {
            uint32_t initial = G.offsets[i + 1] - G.offsets[i];
            if (initial == 0) { insert(i); }
            bool is_mode = (initial > MODE_THRESHOLD);
            if (is_mode) mode.mark_mode(i);
            return Counter(initial, is_mode);
        });
    }
};


#undef NUM_BUCKETS
#undef LUT_BLOK
#undef LUT_MASK
#undef LUT_TYPE
#undef LUT_INIT
#undef LUT_MAXI
#undef LUT_SIZE
#undef USE_1_BUCKETS
#undef USE_2_BUCKETS
#undef USE_4_BUCKETS
#undef USE_8_BUCKETS
#undef USE_16_BUCKETS
#undef MODE_THRESHOLD
