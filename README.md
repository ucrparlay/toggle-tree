# Toggle Tree

A parallel data structure. Functions as a `vertex subset` for shared-memory graph algorithms.

Requires: Linux (x86_64 with the BMI2 instruction set), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## 1. Benchmarking

Benchmarks in Toggle Tree are designed to produce bit-for-bit identical output with the GBBS/PASGAL baseline implementations.

### 1.1. Environment Setup

No external dependencies are required for running ToT/PASGAL benchmarks.
They only rely on parlaylib, and we have vendored it for benchmarking.

Benchmarking the GBBS baseline requires `Bazel 7.7.1`.
Bazel is only used to build GBBS baselines, and has nothing to do with Toggle Tree.

### 1.2. Configuration

```bash
[benchmark_utils/scripts]$ cp example_of_config.sh config.sh
```

Then configure the number of threads, your graph directory, and the graphs to be benchmarked in the directory.

If you don't have any graphs in your configured directory, you can use the following command to download them: 

```bash
[benchmark_utils/generate_graph]$ bash ./download_graphs.sh
```

If you don't have `*_wghlog.bin` weighted graphs in your configured directory, you can use the following command to generate them: 

```bash
[benchmark_utils/generate_graph/generate_weight]$ bash ./bench.sh
```

### 1.3. Benchmarking

Each folder in `benchmarks/` is a separate implementation, 
you can use `bash ./bench.sh` in each of them to run all the graphs, 
or `bash ./test.sh` for a single graph configured in `config.sh`.

Scripts for testing all the algorithms are also provided: 

```bash
[benchmark_utils/scripts]$ ./test_all.sh
[benchmark_utils/scripts]$ ./bench_all.sh
```

After running benchmarks, two `.csv` files will be generated.

`benchmark.csv` contains timing results.

`verify.csv` contains results of using the same hash function on both outputs of GBBS and Toggle Tree. This is intended as a fast sanity check, since writing and comparing full outputs can be impractical at scale. 

### 1.4. Bitwise Verification

Aside from the fast hash-based sanity check, there is a script for bitwise verification.

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
```

To select a specific graph with a specific algorithm, please edit the first few lines of `bitwise_verify.sh`.

## 2. Usage

This is a header-only C++ library. You may add `/include` to the include path when compiling.

Here is a brief overview of what is included in the interface.
For more detailed information about usage, it will be more straightforward to read `include/toggle/detail/active.h` and `include/toggle/detail/frontier.h`.

Note that the actual interface contains more implementation details than the paper's pseudocode.
There is no explicit clear function provided. Instead, it is done by passing a template parameter `<true>` to `for_each()`, so as to save an unnecessary tree traversal. 
From the paper's pseudocode you can discover that actually `clear()` always appear immediately after `for_each()`.

Remember that any `for_each` or `reduce` visit can only remove the vertex itself, do NOT call `remove` on another vertex inside the same Toggle Tree! 
Just like atomic operations do not automatically make your parallel algorithm correct, 
we do not build some complex framework to guarantee your parallel algorithm is correct. 
Here, performance is the first-class citizen, not encapsulation.

```text
toggle::Frontier frontier(n) // empty by default
toggle::Active active(n)     // full by default
```

| Return Value | Active | Frontier |
| --- | --- | --- | 
| bool | empty() | empty() | 
| bool | contains(i) | contains(i) | 
| void | insert(i) | insert(i) | 
| void | remove(i) | remove(i) | 
| bool | -- | empty_next() | 
| bool | -- | contains_next(i) | 
| void | -- | insert_next(i) | 
| void | -- | remove_next(i) | 
| bool | -- | advance_to_next() | 
| void | for_each(F) | for_each(F) | 
| T | reduce<T,identity>(F, combine) | reduce<T,identity>(F, combine) | 

`Frontier.for_each()` by default removes all the elements after execution, while `Active.for_each()` keeps them. You can also manually make it clear or preserve by passing the template parameter `<true>` or `<false>`.

Below are some frequently used reduce functions, provided for convenience:

| Return Value | Active | Frontier |
| --- | --- | --- | 
| uint64_t | approximate_vertex() | approximate_vertex() | 
| uint64_t | reduce_vertex() | reduce_vertex() | 
| uint64_t | reduce_edge(G) | reduce_edge(G) | 
| T | reduce_max(sequence) | reduce_max(sequence) | 
| T | reduce_min(sequence) | reduce_min(sequence) | 

`approximate_vertex()` does reduction on the last but not least layer, and is more efficient than `reduce_vertex()`.

There is also `include/toggle/detail/internal/index_map.h`, it provided `repair()` and `extract_min()` separately, so that rho-stepping can also fit into it in the future.
