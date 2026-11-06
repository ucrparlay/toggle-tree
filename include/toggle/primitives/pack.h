// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>
#include "../parallel/parallel.h"
#include "../data_structures/array.h"

namespace toggle {

namespace internal {

template <class Condition>
inline uint64_t pack_mask_word(uint64_t base, uint64_t end, Condition& condition) noexcept {
    uint64_t m = 0;
    if (end - base >= 64) { for (uint64_t i = 0; i < 64; i++) { m |= uint64_t(bool(condition(base + i))) << i; } }
    else { for (uint64_t i = 0, ie = end - base; i < ie; i++) { m |= uint64_t(bool(condition(base + i))) << i; } }
    return m;
}

template <class T, class Condition, class F, class GetOutput>
inline uint64_t pack_impl(uint64_t start, uint64_t end, Condition& condition, F& f, GetOutput&& get_output) noexcept {
    uint64_t n = end - start;
    uint64_t num_words = (n + 63) / 64;
    if (n < 8192) {
        auto mask = toggle::Array<uint64_t>(num_words);
        uint64_t total = 0;
        for (uint64_t w = 0; w < num_words; ++w) { mask[w] = pack_mask_word(start + w * 64, end, condition); total += std::popcount(mask[w]); }
        T* output = get_output(total);
        uint64_t offset = 0;
        for (uint64_t w = 0; w < num_words; ++w) {
            uint64_t m = mask[w], base = start + w * 64;
            while (m != 0) { output[offset++] = f(base + std::countr_zero(m)); m &= m - 1; }
        }
        return total;
    }
    uint64_t num_blocks = std::min<uint64_t>(toggle::num_workers() * 16, (num_words + 15) / 16);
    uint64_t block_words = (num_words + num_blocks - 1) / num_blocks;
    auto mask = toggle::Array<uint64_t>(num_words);
    auto counts = toggle::Array<uint64_t>(num_blocks, [&](uint64_t block) {
        uint64_t wl = block * block_words, wr = std::min(num_words, wl + block_words), count = 0;
        for (uint64_t w = wl; w < wr; ++w) {
            uint64_t m = pack_mask_word(start + w * 64, end, condition);
            mask[w] = m; count += std::popcount(m);
        }
        return count;
    }, 1);
    uint64_t total = 0;
    for (uint64_t block = 0; block < num_blocks; block++) { total += counts[block]; counts[block] = total - counts[block]; }
    T* output = get_output(total);
    toggle::parallel_for(0, num_blocks, [&](uint64_t block) {
        uint64_t wl = block * block_words, wr = std::min(num_words, wl + block_words);
        uint64_t offset = counts[block];
        for (uint64_t w = wl; w < wr; ++w) {
            uint64_t m = mask[w], base = start + w * 64;
            while (m != 0) { output[offset++] = f(base + std::countr_zero(m)); m &= m - 1; }
        }
    }, 1);
    return total;
}

} // namespace internal

template<class T, class Condition, class F>
toggle::Array<T> pack(uint64_t start, uint64_t end, Condition&& condition, F&& f) noexcept {
    toggle::Array<T> output;
    internal::pack_impl<T>(start, end, condition, f, [&](uint64_t total) {
        output = toggle::Array<T>(total);
        return output.begin();
    });
    return output;
}

template<class T, class Condition, class F>
uint64_t pack_into(toggle::Array<T>& output, uint64_t start, uint64_t end, Condition&& condition, F&& f) noexcept {
    return internal::pack_impl<T>(start, end, condition, f, [&](uint64_t) { return output.begin(); });
}

} // namespace toggle
