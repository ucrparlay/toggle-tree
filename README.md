# ParSet: High-Performance Concurrent Data Structures with Set Semantics

Provides high-performance, composable, and parallel foundational data structures and operation interfaces for graph algorithms based on vertex subsets.

Requires Linux, GCC >= 9 (C++17)
Supports graphs with up to 2³² vertices and 2⁶⁴ edges.

# Usage

## Compilation and Linking

In your Makefile, add `-I` to link to the library's `include` directory, and ensure that you also link to the `parlaylib` include directory. Then you can start using ParSet via:

```c++
#include <ParSet/ParSet.h>
```

For a usage example, see:

```
benchmarks/BFS/ParSet/Makefile
```

## Using ParSet

### Data Structures and Constructors

The core data structure of ParSet is a hierarchical parallel bitmap. Two classes are provided: `Frontier` and `Active`.

- `Frontier` is a double-buffered structure containing two bitmap instances. While operating on one, insertions can be made into the other, and `advance()` swaps the two instances.
- The constructor parameter `n` is the maximum size of the set, typically equal to the number of vertices in the graph.
- `Frontier` is initialized to all zeros by default.
- `Active` is initialized to all ones by default.

```c++
auto active = ParSet::Active(n); 
auto frontier = ParSet::Frontier(n);
```

The following descriptions demonstrate commonly used functionality. For full interfaces, see:

```
include/ParSet/vertex_parallel/frontier.h
include/ParSet/vertex_parallel/active.h
```

### Concurrent Insertion, Deletion, and Packing

ParSet follows set semantics. Multiple insertions of the same value result in only one retained instance.

```c++
active.is_active(x);                  // Whether x exists in active
active.deactivate(x);                 // Remove x from active
frontier.insert(x);                   // Insert x into frontier's next
frontier.advance();                   // Swap internal buffers and return whether non-empty
size_t size = frontier.pack_into(seq);// Pack frontier elements into a parlay::sequence
```

`pack_into` enables ParSet to act as a parallel element collection container.

However, packing is usually not the recommended approach, since ParSet supports direct parallel operations on the set.

### Parallel Operations

ParSet allows efficient traversal of set elements without packing. `parallel_do` is the core method for building parallel graph algorithms:

```c++
active.parallel_do([&](size_t s) { ... });     // Parallel operation on each element in active
frontier.parallel_do([&](size_t s) { ... });   // Parallel operation on each element in frontier and clear frontier
```

By default:

- `active.parallel_do` preserves elements after execution.
- `frontier.parallel_do` clears the frontier after execution.

During `frontier.parallel_do`, it is allowed to call `frontier.insert()` (which inserts into the other buffer). The current buffer is cleared. After calling `frontier.advance()`, the cleared buffer becomes the new `next` buffer for future insertions.

This design efficiently supports round-based advancement semantics.

`parallel_do` accepts a template parameter `<true/false>`:

```c++
active.parallel_do<true>(...);       // Parallel operation and clear active
frontier.parallel_do<false>(...);    // Parallel operation without clearing frontier
```

### Parallel Reduction

ParSet supports reduction over set elements without packing. Work is always proportional to the current set size. The interfaces are identical for `Active` and `Frontier`.

```c++
active.reduce(
    [&](size_t s) { ... },               // How each element provides a value
    [&](uint64_t l, uint64_t r) { ... }, // How values are combined
);
```

ParSet only supports reduction over positive integers. Zero is used as a sentinel value. Therefore, when computing a minimum, avoid returning zero from the element function.

For convenience, common reductions are provided:

```c++
uint64_t res = active.reduce_max(sequence);  // Maximum over sequence[s] for s in active
uint64_t res = active.reduce_min(sequence);
uint64_t res = active.reduce_sum(sequence);
uint64_t res = active.reduce_vertex();       // Number of elements in the set
uint64_t res = active.reduce_edge(graph);    // Total number of neighbors of the vertex set
```

### Parallel Reduction with Augment Value Construction

ParSet can preserve the reduction process for further use.

The implementation of `pack` internally performs a sum-reduction on 1 to obtain prefix sums before writing to the sequence.

```c++
// Reduce minimum value and preserve the reduction structure
active.reduce<true>(
    [&] (size_t i) { return array[i]; },
    [&] (uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
);
```

The stored augment values can then be used to quickly extract elements equal to the minimum or maximum.

```c++
// Quickly select elements equal to the minimum value
active.select(
    [&] (size_t i) { return array[i]; }, 
    [&] (size_t s) { ... } // Operation on selected elements
);
```

Convenience functions are also provided:

```c++
active.reduce_and_select_max(array, lambda);
active.reduce_and_select_min(array, lambda);
```

### Parallel Random Pop

Supports prefix-doubling style algorithms without packing.

```c++
active.pop(k, frontier, lambda); // Randomly pop k elements into frontier
```

# Testing

Enter the `benchmark_utils` directory.

Copy the configuration file:

```bash
cp example_of_config.sh config.sh
```

Modify `config.sh`:

Set `BIN_DIR` to the path where graph data is stored.

Run an algorithm, for example:

```bash
cd benchmarks/BFS/ParSet
./bench.sh
```

To test using the built-in graph:

```bash
./test.sh
```

`HepPh_sym.bin` is included and requires no external data.