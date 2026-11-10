// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include "../parallel/macros.h"
#include "../parallel/parallel.h"
#include "../parallel/word.h"
#include "../data_structures/array.h"
#include "random.h"

namespace toggle {

namespace internal {

template <class RandomIt, class LeftWrong, class RightWrong>
inline void block_partition(RandomIt& l, RandomIt& r, LeftWrong left_wrong, RightWrong right_wrong) {
    uint64_t left_bits = 0, right_bits = 0;
    while (r - l >= 128) {
        RandomIt rb = r - 64;
        if (left_bits == 0) { for (uint64_t i = 0; i < 64; i++) { left_bits |= uint64_t(left_wrong(l[i])) << i; } }
        if (right_bits == 0) { for (uint64_t i = 0; i < 64; i++) { right_bits |= uint64_t(right_wrong(rb[i])) << i; } }
        while (left_bits && right_bits) {
            std::iter_swap(l + std::countr_zero(left_bits), rb + std::countr_zero(right_bits));
            left_bits &= left_bits - 1; right_bits &= right_bits - 1;
        }
        if (left_bits == 0) { l += 64; }
        if (right_bits == 0) { r -= 64; }
    }
}

template <class RandomIt, class GoesRight>
inline RandomIt small_partition(RandomIt l, RandomIt r, GoesRight goes_right) {
    uint64_t n = r - l, n0 = std::min<uint64_t>(n, 64), n1 = n - n0;
    uint64_t lo = 0, hi = 0;
    for (uint64_t i = 0; i < n0; i++) { lo |= uint64_t(goes_right(l[i])) << i; }
    for (uint64_t i = 0; i < n1; i++) { hi |= uint64_t(goes_right(l[64 + i])) << i; }
    uint64_t cut = n - std::popcount(lo) - std::popcount(hi);
    auto low_bits = [](uint64_t k) { return k >= 64 ? ~uint64_t(0) : (uint64_t(1) << k) - 1; };
    uint64_t left_lo = low_bits(cut), left_hi = cut > 64 ? low_bits(cut - 64) : 0;
    uint64_t a_lo = lo & left_lo, a_hi = hi & left_hi;
    uint64_t b_lo = ~lo & low_bits(n0) & ~left_lo, b_hi = ~hi & low_bits(n1) & ~left_hi;
    auto pair_up = [&](uint64_t& a, RandomIt a_base, uint64_t& b, RandomIt b_base) {
        while (a && b) {
            std::iter_swap(a_base + std::countr_zero(a), b_base + std::countr_zero(b));
            a &= a - 1; b &= b - 1;
        }
    };
    pair_up(a_lo, l, b_lo, l); pair_up(a_lo, l, b_hi, l + 64);
    pair_up(a_hi, l + 64, b_lo, l); pair_up(a_hi, l + 64, b_hi, l + 64);
    return l + cut;
}

template <class RandomIt, class Compare>
inline std::pair<RandomIt, RandomIt> partition3(RandomIt first, RandomIt last, Compare& comp) {
    auto pivot = *first;
    auto lt = [&](const auto& x) { return comp(x, pivot); };
    auto ge = [&](const auto& x) { return !comp(x, pivot); };
    auto gt = [&](const auto& x) { return comp(pivot, x); };
    auto le = [&](const auto& x) { return !comp(pivot, x); };
    RandomIt l = first + 1, r = last;
    block_partition(l, r, ge, lt);
    RandomIt mid = small_partition(l, r, ge);
    l = mid; r = last;
    block_partition(l, r, gt, le);
    RandomIt equal_end = small_partition(l, r, gt);
    std::iter_swap(first, mid - 1);
    return {mid - 1, equal_end};
}


template <class RandomIt, class Compare>
inline void insertion_sort(RandomIt first, RandomIt last, Compare& comp) {
    if (first == last) { return; }
    for (RandomIt i = first + 1; i < last; i++) {
        auto x = std::move(*i);
        RandomIt j = i;
        for (; j > first && comp(x, *(j - 1)); --j) { *j = std::move(*(j - 1)); }
        *j = std::move(x);
    }
}

template <class RandomIt, class Compare>
inline void move_median_to_first(RandomIt first, RandomIt a, RandomIt b, RandomIt c, Compare& comp) {
    if (comp(*a, *b)) {
        if (comp(*b, *c)) { std::iter_swap(first, b); }
        else if (comp(*a, *c)) { std::iter_swap(first, c); }
        else { std::iter_swap(first, a); }
    }
    else if (comp(*a, *c)) { std::iter_swap(first, a); }
    else if (comp(*b, *c)) { std::iter_swap(first, c); }
    else { std::iter_swap(first, b); }
}

template <class RandomIt, class Compare>
inline void network_sort32(RandomIt first, RandomIt last, Compare& comp) {
    using T = typename std::iterator_traits<RandomIt>::value_type;
    static constexpr uint8_t network[185][2] = {
        {0, 1 }, {2, 3 }, {4, 5 }, {6, 7 }, {8, 9 }, {10,11}, {12,13}, {14,15}, {16,17}, {18,19}, {20,21}, {22,23}, {24,25}, {26,27}, {28,29}, {30,31},
        {0, 2 }, {1, 3 }, {4, 6 }, {5, 7 }, {8, 10}, {9, 11}, {12,14}, {13,15}, {16,18}, {17,19}, {20,22}, {21,23}, {24,26}, {25,27}, {28,30}, {29,31},
        {0, 4 }, {1, 5 }, {2, 6 }, {3, 7 }, {8, 12}, {9, 13}, {10,14}, {11,15}, {16,20}, {17,21}, {18,22}, {19,23}, {24,28}, {25,29}, {26,30}, {27,31},
        {0, 8 }, {1, 9 }, {2, 10}, {3, 11}, {4, 12}, {5, 13}, {6, 14}, {7 ,15}, {16,24}, {17,25}, {18,26}, {19,27}, {20,28}, {21,29}, {22,30}, {23,31},
        {0, 16}, {1, 8 }, {2, 4 }, {3, 12}, {5, 10}, {6, 9 }, {7, 14}, {11,13}, {15,31}, {17,24}, {18,20}, {19,28}, {21,26}, {22,25}, {23,30}, {27,29},
        {1, 2 }, {3, 5 }, {4, 8 }, {6, 22}, {7, 11}, {9, 25}, {10,12}, {13,14}, {17,18}, {19,21}, {20,24}, {23,27}, {26,28}, {29,30}, 
        {1, 17}, {2, 18}, {3, 19}, {4, 20}, {5, 10}, {7, 23}, {8, 24}, {11,27}, {12,28}, {13,29}, {14,30}, {21,26},
        {3, 17}, {4, 16}, {5, 21}, {6, 18}, {7, 9 }, {8, 20}, {10,26}, {11,23}, {13,25}, {14,28}, {15,27}, {22,24},
        {1, 4 }, {3, 8 }, {5, 16}, {7, 17}, {9, 21}, {10,22}, {11,19}, {12,20}, {14,24}, {15,26}, {23,28}, {27,30},
        {2, 5 }, {7, 8 }, {9, 18}, {11,17}, {12,16}, {13,22}, {14,20}, {15,19}, {23,24}, {26,29},
        {2, 4 }, {6, 12}, {9, 16}, {10,11}, {13,17}, {14,18}, {15,22}, {19,25}, {20,21}, {27,29},
        {5, 6 }, {8, 12}, {9, 10}, {11,13}, {14,16}, {15,17}, {18,20}, {19,23}, {21,22}, {25,26},
        {3, 5 }, {6, 7 }, {8, 9 }, {10,12}, {11,14}, {13,16}, {15,18}, {17,20}, {19,21}, {22,23}, {24,25}, {26,28},
        {3, 4 }, {5, 6 }, {7, 8 }, {9, 10}, {11,12}, {13,14}, {15,16}, {17,18}, {19,20}, {21,22}, {23,24}, {25,26}, {27,28},
    };
    uint64_t n = last - first;
    if (n < 2) { return; }
    T v[32];
    T top = first[0];
    for (uint64_t i = 0; i < n; i++) { v[i] = first[i]; top = comp(top, v[i]) ? v[i] : top; }
    for (uint64_t i = n; i < 32; i++) { v[i] = top; }
    #pragma GCC unroll 1024
    for (uint64_t k = 0; k < 185; k++) {
        T& a = v[network[k][0]]; T& b = v[network[k][1]];
        bool swap = comp(b, a);
        T lo = swap ? b : a, hi = swap ? a : b;
        a = lo; b = hi;
    }
    for (uint64_t i = 0; i < n; i++) { first[i] = v[i]; }
}

template <class RandomIt, class Compare>
void introsort_loop(RandomIt first, RandomIt last, uint64_t depth, Compare& comp) {
    using T = typename std::iterator_traits<RandomIt>::value_type;
    constexpr bool use_network = std::is_arithmetic_v<T> && (
        std::is_same_v<Compare, std::less<T>> || std::is_same_v<Compare, std::less<>> ||
        std::is_same_v<Compare, std::greater<T>> || std::is_same_v<Compare, std::greater<>>
    );
    constexpr int64_t leaf_size = use_network ? 32 : 16;
    while (last - first > leaf_size) {
        if (depth == 0) { std::partial_sort(first, last, last, comp); return; }
        depth--;
        move_median_to_first(first, first + 1, first + (last - first) / 2, last - 1, comp);
        auto [equal_begin, equal_end] = partition3(first, last, comp);
        introsort_loop(equal_end, last, depth, comp);
        last = equal_begin;
    }
    if constexpr (use_network) { network_sort32(first, last, comp); }
    else { insertion_sort(first, last, comp); }
}

template <class T> struct is_arithmetic_pair : std::false_type {};
template <class A, class B> struct is_arithmetic_pair<std::pair<A, B>> : std::bool_constant<std::is_arithmetic_v<A> && std::is_arithmetic_v<B>> {};
template <class T, class Compare>
constexpr bool is_pair_less_v = is_arithmetic_pair<T>::value && (std::is_same_v<Compare, std::less<T>> || std::is_same_v<Compare, std::less<>>);
template <class T, class Compare>
constexpr bool is_pair_greater_v = is_arithmetic_pair<T>::value && (std::is_same_v<Compare, std::greater<T>> || std::is_same_v<Compare, std::greater<>>);

template <bool Greater>
struct branchless_pair_compare {
    template <class P>
    bool operator()(const P& x, const P& y) const {
        const P& a = Greater ? y : x;
        const P& b = Greater ? x : y;
        return bool(int(a.first < b.first) | (int(a.first == b.first) & int(a.second < b.second)));
    }
};

template <class RandomIt, class Compare = std::less<typename std::iterator_traits<RandomIt>::value_type>>
void sequential_sort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
    using T = typename std::iterator_traits<RandomIt>::value_type;
    uint64_t depth = 2 * (std::bit_width(uint64_t(last - first)) - 1);
    if constexpr (toggle::internal::is_pair_less_v<T, Compare>) {
        toggle::internal::branchless_pair_compare<false> c;
        toggle::internal::introsort_loop(first, last, depth, c);
    }
    else if constexpr (toggle::internal::is_pair_greater_v<T, Compare>) {
        toggle::internal::branchless_pair_compare<true> c;
        toggle::internal::introsort_loop(first, last, depth, c);
    }
    else { toggle::internal::introsort_loop(first, last, depth, comp); }
}

} // namespace internal

template <class RandomIt, class T = typename std::iterator_traits<RandomIt>::value_type, class Compare = std::less<T>>
void parallel_sort(RandomIt start, RandomIt end, Compare comp = Compare{}) {
    uint64_t n = end - start;
    if (n < 8192) { internal::sequential_sort(start, end, comp); return; }
    auto sqrt = [](uint64_t x) { return 1ULL << (std::bit_width(x - 1)) / 2; };
    uint64_t num_buckets = toggle::num_workers() * std::max<uint64_t>(1, sqrt(n*sizeof(T)/128)/toggle::num_workers());
    uint64_t block_size = (n + num_buckets - 1) / num_buckets;
    uint64_t num_blocks = (n + block_size - 1) / block_size;
    uint64_t num_samples = std::min<uint64_t>(n, num_buckets * 4);
    auto sample = toggle::Array<T>(num_samples, [&](uint64_t i) { return start[hash64(i) % n]; });
    internal::sequential_sort(sample.begin(), sample.end(), comp);
    auto splitters = toggle::Array<T>(num_buckets - 1, [&](uint64_t i) { return sample[std::min<uint64_t>((i + 1) * num_samples / num_buckets, num_samples - 1)]; });
    auto buffer = toggle::Array<T>(n);
    auto counts = toggle::Array<uint64_t>(num_blocks * num_buckets);
    auto source_offsets = toggle::Array<uint64_t>(num_blocks * num_buckets + 1);
    toggle::parallel_for(0, num_blocks, [&](uint64_t block) {
        uint64_t local_start = block * block_size;
        uint64_t local_end = std::min<uint64_t>(n, local_start + block_size);
        internal::sequential_sort(start + local_start, start + local_end, comp);
        if constexpr (std::is_trivially_copyable_v<T>) { toggle::stream_copy(buffer.begin() + local_start, start + local_start, local_end - local_start); }
        else { std::move(start + local_start, start + local_end, buffer.begin() + local_start); }
        uint64_t base = block * num_buckets;
        uint64_t bucket = 0, current = 0;
        source_offsets[base] = local_start;
        for (uint64_t i = local_start; i < local_end; i++) {
            std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&> value = std::is_trivially_copyable_v<T> ? start[i] : buffer[i];
            while (bucket < splitters.size() && comp(splitters[bucket], value)) { bucket++; }
            uint64_t target = bucket + (bucket < splitters.size() && !comp(value, splitters[bucket]));
            if (target != current) {
                counts[base + current] = i - source_offsets[base + current];
                for (uint64_t b = current + 1; b <= target; b++) { source_offsets[base + b] = i; counts[base + b] = 0; }
                current = target;
            }
        }
        counts[base + current] = local_end - source_offsets[base + current];
        for (uint64_t b = current + 1; b < num_buckets; b++) { source_offsets[base + b] = local_end; counts[base + b] = 0; }
        toggle::stream_fence();
    }, 1);
    source_offsets[num_blocks * num_buckets] = n;
    auto bucket_offsets = toggle::internal::histogram(num_buckets, num_blocks, counts);
    auto transpose_runs = [&](auto&& self, uint64_t row_begin, uint64_t row_end, uint64_t col_begin, uint64_t col_end) -> void {
        uint64_t rows = row_end - row_begin, cols = col_end - col_begin;
        if (rows * cols < 8192) {
            toggle::parallel_for(col_begin, col_end, [&](uint64_t j) {
                for (uint64_t i = row_begin; i < row_end; i++) {
                    uint64_t s = source_offsets[i * num_buckets + j], d = counts[i * num_buckets + j];
                    uint64_t len = source_offsets[i * num_buckets + j + 1] - s;
                    if constexpr (std::is_trivially_copyable_v<T>) { for (uint64_t k = 0; k < len; k++) { toggle::stream_element(start + d + k, buffer[s + k]); } }
                    else { std::move(buffer.begin() + s, buffer.begin() + s + len, start + d); }
                }
                toggle::stream_fence();
            }, 1);
        }
        else if (cols > rows) {
            uint64_t mid = col_begin + cols / 2;
            toggle::parallel_do(
                [&] { self(self, row_begin, row_end, col_begin, mid); },
                [&] { self(self, row_begin, row_end, mid, col_end); }
            );
        }
        else {
            uint64_t mid = row_begin + rows / 2;
            toggle::parallel_do(
                [&] { self(self, row_begin, mid, col_begin, col_end); },
                [&] { self(self, mid, row_end, col_begin, col_end); }
            );
        }
    };
    transpose_runs(transpose_runs, 0, num_blocks, 0, num_buckets);
    toggle::parallel_for(0, num_buckets, [&](uint64_t bucket) {
        bool all_equal = bucket > 0 && bucket < splitters.size() && !comp(splitters[bucket - 1], splitters[bucket]) && !comp(splitters[bucket], splitters[bucket - 1]);
        if (!all_equal) {
            internal::sequential_sort(start + bucket_offsets[bucket], start + bucket_offsets[bucket + 1], comp);
        }
    }, 1);
}

template <class T, class Compare = std::less<T>>
void parallel_sort(toggle::Array<T>& A, Compare comp = Compare{}) {
    toggle::parallel_sort(A.begin(), A.begin() + A.size(), std::move(comp));
}

} // namespace toggle
