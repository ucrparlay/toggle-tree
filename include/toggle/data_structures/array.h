// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>
#include "../parallel/parallel.h"

namespace toggle {

namespace internal {

struct Pool {
    static constexpr size_t Threshold1 = 6;
    static constexpr size_t Threshold2 = 16;

    struct alignas(64) PCache { std::array<void*, sizeof(size_t) * 8 - Threshold1> cache{}; };
    alignas(64) std::vector<PCache> local_cache;
    alignas(64) std::array<void*, sizeof(size_t) * 8 - Threshold2> global_cache{};
    alignas(64) bool global_lock;

    Pool() noexcept : local_cache(toggle::num_workers()), global_lock(false) {}
    ~Pool() noexcept {
        auto free_chain = [](void *head) { while (head != nullptr) { void *next = *static_cast<void **>(head); ::operator delete(head, std::align_val_t{64}); head = next; } };
        for (auto &lists : local_cache) for (void *head : lists.cache) free_chain(head);
        for (void *head : global_cache) free_chain(head);
    }

    bool try_lock_global() noexcept {
        bool old = false;
        return std::atomic_ref<bool>(global_lock).compare_exchange_strong(old, true, std::memory_order_acquire, std::memory_order_relaxed);
    }
    void unlock_global() noexcept { std::atomic_ref<bool>(global_lock).store(false, std::memory_order_release); }

    void *allocate(size_t bytes) noexcept {
        size_t bucket = bytes <= (1ULL << Threshold1) ? Threshold1 : std::bit_width(bytes - 1);
        void *&local_slot = local_cache[toggle::worker_id()].cache[bucket - Threshold1];
        if (local_slot != nullptr) {
            void *p = local_slot;
            local_slot = *static_cast<void **>(p);
            return p;
        }
        if (bucket >= Threshold2 && try_lock_global()) {
            void *p = global_cache[bucket - Threshold2];
            if (p != nullptr) global_cache[bucket - Threshold2] = *static_cast<void **>(p);
            unlock_global();
            if (p != nullptr) return p;
        }
        return ::operator new(1ULL << bucket, std::align_val_t{64});
    }

    void deallocate(void *p, size_t bytes) noexcept {
        size_t bucket = bytes <= (1ULL << Threshold1) ? Threshold1 : std::bit_width(bytes - 1);
        void *&local_slot = local_cache[toggle::worker_id()].cache[bucket - Threshold1];
        if (bucket < Threshold2 || local_slot == nullptr) {
            *static_cast<void **>(p) = local_slot;
            local_slot = p;
            return;
        }
        if (try_lock_global()) {
            *static_cast<void **>(p) = global_cache[bucket - Threshold2];
            global_cache[bucket - Threshold2] = p;
            unlock_global();
            return;
        }
        ::operator delete(p, std::align_val_t{64});
    }
};

extern inline Pool& internal_get_pool() noexcept { static Pool pool; return pool; }
extern inline Pool& get_pool() noexcept {
    static thread_local Pool* pool = []() noexcept {
        Pool* p = &internal_get_pool();
        if (toggle::worker_id() >= toggle::num_workers()) std::abort();
        return p;
    }();
    return *pool;
}

} // namespace internal

template <typename T>
struct Array {
    static_assert(std::is_same_v<std::remove_cv_t<T>, T>);
    static_assert(std::is_same_v<std::decay_t<T>, T>);
    static_assert(std::is_destructible_v<T>);
    using value_type = T;

    T *data_;
    uint64_t size_;

    Array() noexcept {
        size_ = 0; data_ = nullptr;
    }

    Array(Array &&other) noexcept {
        size_ = other.size_; other.size_ = 0;
        data_ = other.data_; other.data_ = nullptr;
    }

    Array(uint64_t n) noexcept {
        if (n > (std::numeric_limits<size_t>::max)() / sizeof(T)) std::abort();
        size_ = n; data_ = (n == 0) ? nullptr : static_cast<T *>(internal::get_pool().allocate(n * sizeof(T)));
        if constexpr (!std::is_trivially_destructible_v<T>) {
            toggle::parallel_for(0, n, [&](uint64_t i) { std::construct_at(data_ + i); }, std::max<uint64_t>(1, 16384 / sizeof(T)));
        }
    }

    Array(uint64_t n, const T &value) noexcept {
        if (n > (std::numeric_limits<size_t>::max)() / sizeof(T)) std::abort();
        size_ = n; data_ = (n == 0) ? nullptr : static_cast<T *>(internal::get_pool().allocate(n * sizeof(T)));
        toggle::parallel_for(0, n, [&](uint64_t i) { std::construct_at(data_ + i, value); }, std::max<uint64_t>(1, 16384 / sizeof(T)));
    }

    Array(const Array &other) noexcept {
        size_ = other.size_; data_ = size_ == 0 ? nullptr : static_cast<T *>(internal::get_pool().allocate(size_ * sizeof(T)));
        toggle::parallel_for(0, size_, [&](uint64_t i) { std::construct_at(data_ + i, other.data_[i]); }, std::max<uint64_t>(1, 16384 / sizeof(T)));
    }

    template <class F> requires std::invocable<F&, uint64_t> && std::constructible_from<T, std::invoke_result_t<F&, uint64_t>>
    Array(uint64_t n, F&& f, uint64_t granularity = 1024) noexcept {
        if (n > (std::numeric_limits<size_t>::max)() / sizeof(T)) std::abort();
        size_ = n; data_ = (n == 0) ? nullptr : static_cast<T *>(internal::get_pool().allocate(n * sizeof(T)));
        toggle::parallel_for(0, n, [&](uint64_t i) { std::construct_at(data_ + i, f(i)); }, granularity);
    }

    Array &operator=(Array &&other) noexcept {
        if (this == &other) return *this;
        if constexpr (!std::is_trivially_destructible_v<T>) { for (uint64_t i = size_; i != 0; --i) std::destroy_at(data_ + i - 1); }
        if (data_ != nullptr) internal::get_pool().deallocate(data_, size_ * sizeof(T));
        data_ = other.data_; other.data_ = nullptr;
        size_ = other.size_; other.size_ = 0;
        return *this;
    }

    ~Array() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) { for (uint64_t i = size_; i != 0; --i) std::destroy_at(data_ + i - 1); }
        if (data_ != nullptr) internal::get_pool().deallocate(data_, size_ * sizeof(T));
        size_ = 0; data_ = nullptr;
    }

    uint64_t size() const noexcept { return size_; }
    T *begin() noexcept { return data_; }
    const T *begin() const noexcept { return data_; }
    T *end() noexcept { return data_ + size_; }
    const T *end() const noexcept { return data_ + size_; }
    T &operator[](uint64_t i) noexcept { return data_[i]; }
    const T &operator[](uint64_t i) const noexcept { return data_[i]; }
    T &back() noexcept { return data_[size() - 1]; }

    void stream_store(uint64_t i, const void* src) noexcept {
        static_assert(std::is_trivially_copyable_v<T> && 64 % sizeof(T) == 0);
        #if !defined(TOGGLE_TREE_WEAK_MEMORY) && defined(__AVX512F__)
            using V = long long __attribute__((vector_size(64), may_alias));
            V value; __builtin_memcpy(&value, src, sizeof(value));
            #if defined(__clang__)
                __builtin_nontemporal_store(value, reinterpret_cast<V*>(data_ + i));
            #else
                __builtin_ia32_movntdq512(reinterpret_cast<V*>(data_ + i), value);
            #endif
        #else
            __builtin_memcpy(data_ + i, src, 64);
        #endif
    }

    static void store_fence() noexcept {
        #if !defined(TOGGLE_TREE_WEAK_MEMORY) && defined(__AVX512F__)
            __builtin_ia32_sfence();
        #endif
    }

    template <int Write = 1>
    void prefetch([[maybe_unused]] uint64_t i) noexcept {
        #ifndef TOGGLE_TREE_WEAK_MEMORY
            __builtin_prefetch(data_ + i, Write, 1);
        #endif
    }
};

} // namespace toggle
