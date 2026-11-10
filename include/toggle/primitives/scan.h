// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <type_traits>
#include "../parallel/macros.h"
#include "../parallel/parallel.h"
#include "../parallel/word.h"
#include "../data_structures/array.h"

namespace toggle {

template<auto Combine, auto Identity, class F>
toggle::Array<decltype(Identity)> scan(uint64_t start, uint64_t end, F&& f) noexcept {
    using T = decltype(Identity);
    uint64_t n = end - start;
    auto output = toggle::Array<T>(n+1);
    if (n < 8192) {
        T total = Identity;
        for (uint64_t i = 0; i < n; i++) {
            output[i] = total;
            total = Combine(total, f(start + i));
        }
        output[n] = total;
        return output;
    }
    uint64_t block_size = 2048;
    uint64_t num_blocks = (n + block_size - 1) / block_size;
    auto offsets = toggle::Array<T>(num_blocks+1, [&](uint64_t block) {
        if (block == num_blocks) return T(Identity);
        uint64_t local_start = block * block_size;
        uint64_t local_end = std::min(n, local_start + block_size);
        T total = Identity;
        for (uint64_t i = local_start; i < local_end; i++) { total = Combine(total, f(start + i)); }
        return total;
    }, 1);
    T total = Identity;
    for (uint64_t i = 0; i <= num_blocks; i++) {
        T tmp = offsets[i];
        offsets[i] = total;
        total = Combine(total, tmp);
    }
    output[n] = total;
    toggle::parallel_for(0, num_blocks, [&](uint64_t block) {
        uint64_t local_start = block * block_size;
        uint64_t local_end = std::min(n, local_start + block_size);
        T offset = offsets[block];
        constexpr uint64_t width = 64 / std::gcd<uint64_t>(64, sizeof(T));   // width 个元素正好是 64B 的整数倍
        for (uint64_t i = local_start; i < local_end; i += width) {
            uint64_t len = std::min(width, local_end - i);
            T values[width];
            for (uint64_t j = 0; j < len; j++) { values[j] = offset; offset = Combine(offset, f(start + i + j)); }
            toggle::stream_copy(output.begin() + i, values, len);
        }
        toggle::stream_fence();
    }, 1);
    return output;
}

template<auto Combine, auto Identity, class Sequence>
toggle::Array<decltype(Identity)> scan(uint64_t start, uint64_t end, Sequence& input) noexcept {
    using T = decltype(Identity);
    return scan<Combine, Identity>(start, end, [&](uint64_t i) { return T(input[i]); });
}

namespace internal {

inline toggle::Array<uint64_t> histogram(uint64_t num_buckets, uint64_t num_blocks, toggle::Array<uint64_t>& counts) noexcept {
    auto bucket_offsets = toggle::Array<uint64_t>(num_buckets + 1);
    uint64_t num_groups = (num_buckets + 8 - 1) / 8;
    toggle::parallel_for(0, num_groups, [&](uint64_t g) {
        uint64_t b0 = g * 8, b1 = std::min(num_buckets, b0 + 8);
        uint64_t acc[8] = {0};
        for (uint64_t block = 0; block < num_blocks; ++block) {
            uint64_t base = block * num_buckets;
            for (uint64_t b = b0; b < b1; ++b) acc[b - b0] += counts[base + b];
        }
        for (uint64_t b = b0; b < b1; ++b) bucket_offsets[b] = acc[b - b0];
    }, 1);
    uint64_t total = 0;
    for (uint64_t bucket = 0; bucket < num_buckets; ++bucket) {
        total += bucket_offsets[bucket];
        bucket_offsets[bucket] = total - bucket_offsets[bucket];
    }
    bucket_offsets[num_buckets] = total;
    toggle::parallel_for(0, num_groups, [&](uint64_t g) {
        uint64_t b0 = g * 8, b1 = std::min(num_buckets, b0 + 8);
        uint64_t off[8];
        for (uint64_t b = b0; b < b1; ++b) off[b - b0] = bucket_offsets[b];
        for (uint64_t block = 0; block < num_blocks; ++block) {
            uint64_t base = block * num_buckets;
            for (uint64_t b = b0; b < b1; ++b) {
                uint64_t index = base + b;
                off[b - b0] += counts[index];
                counts[index] = off[b - b0] - counts[index];
            }
        }
    }, 1);
    return bucket_offsets;
}

} // namespace internal

} // namespace toggle
