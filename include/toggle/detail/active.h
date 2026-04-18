// SPDX-License-Identifier: MIT
#pragma once
#include "internal/index_set.h"

namespace toggle {

struct Active {
    internal::IndexSet active; 
    Active(size_t n, bool init = true) : active(n, init) {}

    inline bool empty() const noexcept { return active.empty(); }
    inline bool contains(size_t i) const noexcept { return active.contains(i); }
    inline void insert(size_t i) noexcept { active.insert(i); }
    inline void remove(size_t i) noexcept { active.remove(i); }

    template <bool Remove = false, class F>
    void for_each(F&& f) { active.for_each<Remove, 64>(f); }

    template<class T, auto Identity, class F, class Combine>
    inline T reduce(F&& f, Combine&& combine){ return active.reduce<T, Identity>(f, combine); }

    inline uint64_t reduce_vertex(){ return active.reduce_vertex(); }
    inline uint64_t approximate_vertex(){ return active.approximate_vertex(); }
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
