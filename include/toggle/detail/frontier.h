// SPDX-License-Identifier: MIT
#pragma once
#include "internal/index_set.h"

namespace toggle {

struct Frontier {
    internal::IndexSet frontier;
    internal::IndexSet next;
    Frontier(size_t n): frontier(n, false), next(n, false) {}

    inline bool empty() const noexcept { return frontier.empty(); }
    inline bool contains(size_t i) const noexcept { return frontier.contains(i); }
    inline void insert(size_t i) noexcept { frontier.insert(i); }
    inline void remove(size_t i) noexcept { frontier.remove(i); }

    inline bool contains_next(size_t i) const noexcept { return next.contains(i); }
    inline void insert_next(size_t i) noexcept { next.insert(i); }
    inline void remove_next(size_t i) noexcept { next.remove(i); }
    inline bool advance_to_next() noexcept { std::swap(frontier, next); return !empty(); }
    
    template <bool Remove = true, class F>
    void for_each(F&& f) { frontier.for_each<Remove, 8>(f); }

    template<class T, auto Identity, class F, class Combine>
    inline T reduce(F&& f, Combine&& combine){ return frontier.reduce<T, Identity>(f, combine); }

    inline uint64_t reduce_vertex(){ return frontier.reduce_vertex(); }
    template <class Graph> inline uint64_t reduce_edge(const Graph& G){ 
        return reduce<uint64_t, 0>(
            [&] (size_t i) { return G.offsets[i+1] - G.offsets[i]; },
            [&] (uint64_t l, uint64_t r) { return l + r; }
        );
    }
    template<class Sequence> inline auto reduce_min(const Sequence& sequence){
        using T = typename Sequence::value_type;
        return reduce<T,std::numeric_limits<T>::max()>(
            [&] (size_t i) { return sequence[i]; },
            [&] (T l,T r) { return (l > r) ? r : l; }
        );
    }
    template<class Sequence> inline auto reduce_max(const Sequence& sequence){
        using T = typename Sequence::value_type;
        return reduce<T,std::numeric_limits<T>::min()>(
            [&] (size_t i) { return sequence[i]; },
            [&] (T l,T r) { return (l > r) ? l : r; }
        );
    }
};

} // namespace toggle
