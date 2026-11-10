// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include "macros.h"

namespace toggle {

constexpr uint64_t pop_count(uint64_t word) noexcept {
    #ifdef TOGGLE_TREE_WEAK_MEMORY
        return uint64_t(std::popcount(word));
    #else
        return uint64_t(__builtin_popcountll(word));
    #endif
}

constexpr uint64_t pop_offset(uint64_t i, uint64_t word) noexcept {
    #if defined(TOGGLE_TREE_WEAK_MEMORY) || !defined(__BMI2__)
        #if !defined(TOGGLE_TREE_WEAK_MEMORY)
            #warning "BMI2 is not enabled, toggle::IndexSet and toggle::IndexMap can not guarantee performance under this setting. Please compile with -mbmi2 or a suitable -march for benchmarking."
        #endif
        while (i--) { word &= word - 1; }
        return uint64_t(std::countr_zero(word));
    #else
        if (std::is_constant_evaluated()) {
            while (i--) { word &= word - 1; }
            return uint64_t(std::countr_zero(word));
        }
        return uint64_t(__builtin_ctzll(__builtin_ia32_pdep_di(1ULL << i, word)));
    #endif
}

constexpr uint64_t log2(uint64_t x) noexcept {
    return std::bit_width(x);
}

constexpr uint64_t glue32(uint64_t word_hi, uint64_t word_lo) noexcept {
    return (word_hi << 32) | word_lo;
}

constexpr uint64_t hi32(uint64_t word) noexcept {
    return word >> 32;
}

constexpr uint64_t lo32(uint64_t word) noexcept {
    return word & 0xFFFFFFFFULL;
}

namespace internal {

#ifndef TOGGLE_TREE_WEAK_MEMORY
    using stream_i32 = int __attribute__((may_alias));
    using stream_i64 = long long __attribute__((may_alias));
    using stream_v16 = long long __attribute__((vector_size(16), may_alias));
    #ifdef __AVX__
        using stream_v32 = long long __attribute__((vector_size(32), may_alias));
    #endif
    #ifdef __AVX512F__
        using stream_v64 = long long __attribute__((vector_size(64), may_alias));
    #endif

    template <class V>
    inline void stream_store_raw(char* d, const char* s) noexcept {
        V v; __builtin_memcpy(&v, s, sizeof(V));
        #if defined(__clang__)
            __builtin_nontemporal_store(v, reinterpret_cast<V*>(d));
        #else
            if constexpr (sizeof(V) == 4) { __builtin_ia32_movnti(reinterpret_cast<V*>(d), v); }
            else if constexpr (sizeof(V) == 8) { __builtin_ia32_movnti64(reinterpret_cast<V*>(d), v); }
            else if constexpr (sizeof(V) == 16) { __builtin_ia32_movntdq(reinterpret_cast<V*>(d), v); }
            #if defined(__AVX__)
                else if constexpr (sizeof(V) == 32) { __builtin_ia32_movntdq256(reinterpret_cast<V*>(d), v); }
            #endif
            #if defined(__AVX512F__)
                else if constexpr (sizeof(V) == 64) { __builtin_ia32_movntdq512(reinterpret_cast<V*>(d), v); }
            #endif
        #endif
    }
#endif

} // namespace internal

template <class It>
inline void stream_element(It dst, const std::iter_value_t<It>& value) noexcept {
    #ifndef TOGGLE_TREE_WEAK_MEMORY
        using T = std::iter_value_t<It>;
        if constexpr (std::contiguous_iterator<It> && std::is_trivially_copyable_v<T> && sizeof(T) % 4 == 0) {
            const char* s = reinterpret_cast<const char*>(std::addressof(value));
            char* d = reinterpret_cast<char*>(std::to_address(dst));
            if constexpr (sizeof(T) % 8 == 0) { for (size_t w = 0; w < sizeof(T); w += 8) { internal::stream_store_raw<internal::stream_i64>(d + w, s + w); } }
            else { for (size_t w = 0; w < sizeof(T); w += 4) { internal::stream_store_raw<internal::stream_i32>(d + w, s + w); } }
        }
        else { *dst = value; }
    #else
        *dst = value;
    #endif
}

template <class OutIt, class InIt>
inline void stream_copy(OutIt dst, InIt src, uint64_t n) noexcept {
    #ifndef TOGGLE_TREE_WEAK_MEMORY
        using T = std::iter_value_t<OutIt>;
        if constexpr (std::contiguous_iterator<OutIt> && std::contiguous_iterator<InIt> && std::is_same_v<T, std::iter_value_t<InIt>> && std::is_trivially_copyable_v<T>) {
            char* d = reinterpret_cast<char*>(std::to_address(dst));
            const char* s = reinterpret_cast<const char*>(std::to_address(src));
            uint64_t bytes = n * sizeof(T);
            uint64_t head = std::min<uint64_t>(bytes, (64 - (reinterpret_cast<uintptr_t>(d) & 63)) & 63);
            std::memcpy(d, s, head); d += head; s += head; bytes -= head;
            for (; bytes >= 64; d += 64, s += 64, bytes -= 64) {
            #if defined(__AVX512F__)
                internal::stream_store_raw<internal::stream_v64>(d, s);
            #elif defined(__AVX__)
                internal::stream_store_raw<internal::stream_v32>(d, s); internal::stream_store_raw<internal::stream_v32>(d + 32, s + 32);
            #else
                for (int k = 0; k < 64; k += 16) { internal::stream_store_raw<internal::stream_v16>(d + k, s + k); }
            #endif
            }
            std::memcpy(d, s, bytes);
        }
        else { std::copy_n(src, n, dst); }
    #else
        std::copy_n(src, n, dst);
    #endif
}

inline void stream_fence() noexcept {
    #ifndef TOGGLE_TREE_WEAK_MEMORY
        __builtin_ia32_sfence();
    #endif
}

} // namespace toggle
