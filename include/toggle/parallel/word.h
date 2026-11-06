// SPDX-License-Identifier: MIT
#pragma once
#include <bit>
#include <cstdint>
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

} // namespace toggle
