# Toggle Tree (ToT)

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## 1. Benchmarking

Benchmarks in Toggle Tree are designed to produce bit-for-bit identical output with the GBBS/PASGAL baseline implementations.

### 1.1. Environment Setup

No external dependencies are required for running ToT/PASGAL benchmarks.
They only rely on parlaylib, and we have vendored it for benchmarking.

Benchmarking GBBS baseline Requires bazel 7.7.1.
Bazel is only used to build GBBS baselines, and has nothing to do with Toggle Tree.

### 1.2. Configuration

```bash
[benchmark_utils/scripts]$ cp example_of_config.sh config.sh
```

Then configure number of threads, directory of graphs and graphs to be benchmarked inside it.

If you don't have any graphs in your configured directory, you can use the following command to download them: 

```bash
[benchmark_utils/generate_graph]$ bash ./download_graphs.sh
```

If you don't have `*_wghlog.bin` weighted graphs in your configured directory, you can use the following command to generate them: 

```bash
[benchmark_utils/generate_graph/generate_weight]$ bash ./bench.sh
```

### 1.3. Benchmarking

Each folder in `benchmarks/` is a seperate implementation, 
you can use `bash ./bench.sh` in them to run all the graphs, 
or `bash ./test.sh` for only one graph configured in `config.sh`.

Scripts for testing all the algorithms are also provided: 

```bash
[benchmark_utils/scripts]$ ./bench_all.sh
```

After running benchmarks, two `.csv` files will be generated.

`benchmark.csv` contains timing results.

`verify.csv` contains results of using the same hash function on both outputs of GBBS and Toggle Tree. This is intended as a fast sanity check, since writing and comparing full outputs can be impractical at scale. 

### 1.4. Bitwise Verification

Aside from fast sanity check with hash, bitwise verification is also provided.

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
```

To select a certain graph with a certain algorithm, please edit the first few lines of `bitwise_verify.sh`.

## 2. Usage

Here is a breif overview of what is included in the interface.
For more detailed infomation of usage, it will be more straitforward to read `include/toggle/detail/active.h` and `include/toggle/detail/frontier.h`.

Note that the actual interface contains some more inplementation details compared to the paper's presudocode.
There is no explicit clear function provided, instead, unconditional clear can be done by passing a template parameter `<true>` on `for_each`, 
and conditional clear is recommended to directly call `remove` inside `for_each`. 
Remember that any `for_each` or `reduce` visit can only remove the vertex itself!
Do NOT call remove on another vertex inside a same Toggle Tree! 

```cpp
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

Frontier.for_each() by default removes all the elements after execution, while Active.reduce() keeps them. You can also manually let it clear or reserve by passing the templete parameter `<true>` or `<false>`

Below are some frequentedly used reduce functions, for convenience:

| Return Value | Active | Frontier |
| --- | --- | --- | 
| uint64_t | reduce_vertex() | reduce_vertex() | 
| uint64_t | reduce_edge(G) | reduce_edge(G) | 
| T | reduce_max(sequence) | reduce_max(sequence) | 
| T | reduce_min(sequence) | reduce_min(sequence) | 

There is also `include/toggle/detail/internal/index_map.h`, it is wrapped in `internal::` to suggest that, if `IndexSet` has better performance than `IndexMap`, it is highly possible that the algorithm is not efficient, such as `WeightedBFS`, which does not include stepping or local search.
