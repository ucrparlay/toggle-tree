# ParSet: Parallel Vertex Set

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## Namespace

`ParSet::`

## Usage

### Active

```cpp
Active(size_t n, bool init = true)
bool empty() const noexcept
bool contains(size_t i) const noexcept
void insert(size_t i) noexcept
void remove(size_t i) noexcept
void for_each(F&& f)
uint64_t reduce(F&& f, Combine&& combine)
parlay::sequence<T> pack()
size_t pack_into(Sequence& out)
void pop(size_t k, F&& f)
uint64_t reduce_vertex()
uint64_t reduce_min(Array& array)
uint64_t adaptive_reduce_min(Array& array, Frontier& frontier)
uint64_t reduce_max(Array& array)
uint64_t reduce_sum(Array& array)
uint64_t reduce_edge(Graph& G)
uint64_t extract_min(Frt& out,Dist& dist)
```

### Frontier

```cpp
Frontier(size_t n)
bool empty() const noexcept
bool contains(size_t i) const noexcept
bool contains_next(size_t i) const noexcept
void insert_next(size_t i) noexcept
void remove_next(size_t i) noexcept
bool advance_to_next() noexcept
void for_each(F&& f)
uint64_t reduce(F&& f, Combine&& combine)
parlay::sequence<T> pack()
size_t pack_into(Sequence& out)
void pop(size_t k, F&& f)
uint64_t reduce_vertex()
uint64_t reduce_min(Array& array)
uint64_t reduce_max(Array& array)
uint64_t reduce_sum(Array& array)
uint64_t reduce_edge(Graph& G)
void repair(Act& active,Array& dist)
```

### edge_parallel

```cpp
template <typename F>
void adaptive_for(size_t start, size_t end, F&& f)
uint32_t adaptive_sum(Graph& G, uint32_t s, size_t l, size_t r, F&& f)
uint32_t adaptive_min(Graph& G, uint32_t s, size_t l, size_t r, F&& f)
bool adaptive_exist(Graph& G, uint32_t s, size_t l, size_t r, F&& f)
```