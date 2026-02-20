
#define MODE_THRESHOLD 1000
#pragma once
#include <new>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#if !defined(USE_1_BUCKETS) && !defined(USE_2_BUCKETS) && !defined(USE_4_BUCKETS) && \
    !defined(USE_8_BUCKETS) && !defined(USE_16_BUCKETS)
    #define USE_4_BUCKETS
#endif
#if defined(USE_16_BUCKETS)
    #define NUM_BUCKETS 16
    #define LUT_BLOK 4
    #define LUT_MASK 15
    #define LUT_TYPE uint64_t
    #define LUT_INIT 0xFEDCBA9876543210
    #define LUT_MAXI 0xFFFFFFFFFFFFFFFF
    #define LUT_SIZE 64
#elif defined(USE_8_BUCKETS)
    #define NUM_BUCKETS 8
    #define LUT_BLOK 3
    #define LUT_MASK 7
    #define LUT_TYPE uint32_t
    #define LUT_INIT 0b111'110'101'100'011'010'001'000
    #define LUT_MAXI 0xFFFFFFFF
    #define LUT_SIZE 24
#elif defined(USE_4_BUCKETS)
    #define NUM_BUCKETS 4
    #define LUT_BLOK 2
    #define LUT_MASK 3
    #define LUT_TYPE uint8_t
    #define LUT_INIT 0b11'10'01'00
    #define LUT_MAXI 0xFF
    #define LUT_SIZE 8
#elif defined(USE_2_BUCKETS)
    #define NUM_BUCKETS 2
    #define LUT_BLOK 1
    #define LUT_MASK 1
    #define LUT_TYPE uint8_t
    #define LUT_INIT 0b1'0
    #define LUT_MAXI 0xFF
    #define LUT_SIZE 2
#elif defined(USE_1_BUCKETS)
    #define NUM_BUCKETS 1
    #define LUT_BLOK 1
    #define LUT_MASK 0
    #define LUT_TYPE uint8_t
    #define LUT_INIT 0b0
    #define LUT_MAXI 0xFF
    #define LUT_SIZE 1
#endif

#ifdef USE_LIBNUMA
  #include <numa.h>
#endif
static thread_local LUT_TYPE worker_offset = LUT_MAXI;
struct alignas(64) IDs { LUT_TYPE next_id; };
inline LUT_TYPE get_offset() {
    if (worker_offset != LUT_MAXI) return worker_offset;
#ifndef USE_LIBNUMA
    static LUT_TYPE next_id = 0;
    LUT_TYPE id = __atomic_fetch_add(&next_id, 1, __ATOMIC_RELAXED);
    return worker_offset = (id & LUT_MASK) * LUT_BLOK;
#else
    static int n_numa = -1;
    if (n_numa == -1) {
        if (numa_available() < 0) n_numa = 1;
        else n_numa = numa_num_configured_nodes();
    }
    int my_numa = 0;
    int cpu = sched_getcpu();
    if (cpu >= 0 && numa_available() >= 0) { my_numa = numa_node_of_cpu(cpu); }

    if (n_numa >= NUM_BUCKETS) {
        LUT_TYPE id = static_cast<LUT_TYPE>(my_numa % NUM_BUCKETS);
        return worker_offset = (id & LUT_MASK) * LUT_BLOK;
    }
    static IDs* ids = nullptr;
    if (ids == nullptr) { 
        ids = static_cast<IDs*>(std::malloc(sizeof(IDs) * n_numa));
        for (int i = 0; i < n_numa; ++i) { ids[i].next_id = 0; }
    }
    
    LUT_TYPE sub_id = __atomic_fetch_add(&ids[my_numa].next_id, 1, __ATOMIC_RELAXED);
    LUT_TYPE start = (NUM_BUCKETS * my_numa) / n_numa;
    LUT_TYPE end   = (NUM_BUCKETS * (my_numa + 1)) / n_numa;
    LUT_TYPE span  = end - start;
    LUT_TYPE id    = start + (sub_id % span);
    return worker_offset = (id & LUT_MASK) * LUT_BLOK;
#endif
}

inline int num_buckets() { return NUM_BUCKETS; }
inline LUT_TYPE initial_bucket_index() {
    LUT_TYPE off = get_offset();
    return (static_cast<LUT_TYPE>(LUT_INIT) >> off) & LUT_MASK;
}
struct alignas(64) LUT  { LUT_TYPE table; bool is_zero; };
struct alignas(64) Bucket { int value; };
struct alignas(64) ZeroCounter {
    LUT    lookup;
    Bucket bucket[NUM_BUCKETS];
    ZeroCounter(int init_count) {
        lookup.table   = LUT_INIT;
        lookup.is_zero = (init_count == 0);
        int base = init_count / NUM_BUCKETS;
        int rem  = init_count % NUM_BUCKETS;
        for (int i = 0; i < NUM_BUCKETS; ++i) bucket[i].value = base + (i < rem ? 1 : 0);
    }
    inline bool is_zero() const noexcept { return __atomic_load_n(&lookup.is_zero, __ATOMIC_RELAXED); }
    inline bool set_zero() noexcept { return !__atomic_exchange_n(&lookup.is_zero, true, __ATOMIC_RELAXED); }
    inline bool decrement() noexcept {
        LUT_TYPE tbl = __atomic_load_n(&lookup.table, __ATOMIC_RELAXED);
        LUT_TYPE off = get_offset();
        LUT_TYPE idx = (tbl >> off) & LUT_MASK;
        for (int i=0; i<NUM_BUCKETS; ) {
            int old = __atomic_fetch_sub(&bucket[idx].value, 1, __ATOMIC_RELAXED);
            if (old > 1) return false;
            if (old == 1) return rewrite_routing(idx);
            do { off = (off + LUT_BLOK) % LUT_SIZE; i++; } while(((tbl >> off) & LUT_MASK) == idx && i < NUM_BUCKETS);
            idx = (tbl >> off) & LUT_MASK;
        }
        return set_zero();
    }
    inline bool seq_decrement() noexcept {
        static thread_local uint32_t rnd = 0x12345678;
        rnd = rnd * 1664525u + 1013904223u;
        int idx = rnd & (NUM_BUCKETS - 1);
        for (int k = 0; k < NUM_BUCKETS; ++k) {
            int i = (idx + k) & (NUM_BUCKETS - 1);
            int old = bucket[i].value--;
            if (old > 1) { return false; }
            if (old == 1) { return rewrite_routing(i); }
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
        if (max_val > 1) { bucket[max_idx].value--; return false; }
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
            int live_cnt = 0; LUT_TYPE live[NUM_BUCKETS];
            for (int i = 0; i < NUM_BUCKETS; ++i) {
                if (__atomic_load_n(&bucket[i].value, __ATOMIC_RELAXED) > 0) { live[live_cnt++] = i; }
            }
            if (live_cnt == 0) { return set_zero(); }
            LUT_TYPE new_tbl = 0;
            for (int i = 0; i < NUM_BUCKETS; ++i) {
                LUT_TYPE b = live[i % live_cnt];
                new_tbl |= b << (LUT_BLOK * i);
            }
            __atomic_store_n(&lookup.table, new_tbl, __ATOMIC_RELAXED);
            LUT_TYPE tbl = __atomic_load_n(&lookup.table, __ATOMIC_RELAXED);
            bool has_idx = false;
            for (LUT_TYPE off = 0; off < LUT_SIZE; off += LUT_BLOK) { if (((tbl >> off) & LUT_MASK) == idx) { has_idx = true; break; } }
            if (!has_idx) { return false; }
        }
    }
    inline uint32_t seq_load() const noexcept {
        uint32_t ret = 0;
        for (int i=0; i<NUM_BUCKETS; i++){
            if (bucket[i].value > 0) ret += bucket[i].value;
        }
        return ret;
    }
    inline void seq_store(uint32_t init_count) noexcept {
        lookup.table   = LUT_INIT;
        lookup.is_zero = (init_count == 0);
        int base = init_count / NUM_BUCKETS;
        int rem  = init_count % NUM_BUCKETS;
        for (int i = 0; i < NUM_BUCKETS; ++i) bucket[i].value = base + (i < rem ? 1 : 0);
    }
};

struct CounterPool {
    inline static ZeroCounter* pool = nullptr;
    inline static uint32_t size = 0;
    inline static uint32_t capacity = 0;
    static void init(uint32_t cap) {
        capacity = cap;
        size_t bytes = size_t(cap) * sizeof(ZeroCounter);
        bytes = (bytes + 63) & ~size_t(63);  
        pool = static_cast<ZeroCounter*>(aligned_alloc(64, bytes));
        if (!pool) abort();
    }
    static inline uint32_t alloc(int init_count) {
        uint32_t idx = __atomic_fetch_add(&size, 1, __ATOMIC_RELAXED);
        new (&pool[idx]) ZeroCounter(init_count);
        return idx;
    }
    static inline ZeroCounter& get(uint32_t idx) {
        return pool[idx];
    }
};

struct Counter {
    uint32_t data;
    Counter(uint32_t init_count, bool color = false) {
        if (init_count >= MODE_THRESHOLD) {
            uint32_t idx = CounterPool::alloc(init_count);
            data = (idx << 2) | (1u << 1) | (color ? 1u : 0u);
        } else {
            data = (init_count << 2) | (color ? 1u : 0u);
        }
    }
    Counter(const Counter& other) noexcept : data(other.data) {}
    inline bool is_color() const noexcept {
        return data & 1u;
    }
    inline bool is_large() const noexcept {
        return data & 2u;
    }
    inline bool decrement() noexcept {
        uint32_t v = data;
        if ((v & ~1u) == 0) return false;
        if (!(v & 2u)) {
            return __atomic_fetch_sub(&data, 4u, __ATOMIC_RELAXED) == 4u;
        }
        if (CounterPool::get(v >> 2).decrement()) {
            data = 0;
            return true;
        }
        return false;
    }
    inline uint16_t seq_load() const noexcept {
        return data >> 2;
    }
    inline void seq_save(uint16_t v) noexcept {
        data = (uint32_t(v) << 2) | (1u << 1) | 1u;
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