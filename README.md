# Toggle Tree (ToT)

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## Benchmarking

Benchmarks in Toggle Tree are designed to produce bit-for-bit identical output with the GBBS baseline implementations.

### Testing Toggle Tree Environment

To make experiments easy to reproduce, no external dependencies are required for running benchmarks.

Since parlaylib is vendored and small graphs are included, this command will directly compile and test all the algorithms: 

```bash
[benchmark_utils/scripts]$ ./test_parset.sh
```

### Testing GBBS Environment

Requires bazel 7.7.1.

Bazel is used to build GBBS baselines. (has nothing to do with Toggle Tree)

All the scripts are well prepared, as long as you get "bazel 7.7.1" when you run "bazel --version", those simple commands will directly compile and test all the baselines: 

```bash
[benchmark_utils/scripts]$ ./test_gbbs.sh
```

### Configure the number of threads, the graph directory, and the graphs to benchmark

```bash
[benchmark_utils/scripts]$ cp example_of_config.sh config.sh
```

Then configure number of threads, directory of graphs and graphs to be benchmarked inside it.

### Bitwise Verification

After successfully performed previous tests and configurations, you may test any benchmarked algorithm on any graph, to see if output of Toggle Tree and GBBS are bit-for-bit identical.

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
```

To select a certain graph with a certain algorithm, please edit the first few lines of `bitwise_verify.sh`.

For the same input graph and deterministic tie-breaking, Toggle Tree produces the exact same per-vertex output label array as GBBS (e.g., the same vertex receives the same BFS level, color id, k-core number, or distance). However, please note that their intermediate execution differs. This is expected: different abstraction levels naturally lead to different algorithmic implementations. Toggle Tree is not a drop-in replacement for GBBS’s VertexSubset. Instead, Toggle Tree uses a straightforward, natural implementation without aggressive, output-preserving optimizations intended to mimic GBBS.

### Benchmarking

```bash
[benchmark_utils/scripts]$ ./bench_all.sh
[benchmark_utils/scripts/BFS]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/Coloring]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/KCore]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/BellmanFord]$ cat verify.csv && echo "------" && cat benchmark.csv
```

After running benchmarks, three `.csv` files will be generated.

`benchmark.csv` contains timing results.

`verify.csv` contains results of using the same hash function on both outputs of GBBS and Toggle Tree. This is intended as a fast sanity check, since writing and comparing full outputs can be impractical at scale. The hash function implementation is in `benchmark_utils/graph/verify.h`. Bitwise verification is also provided elsewhere.

`max.csv` contains the max result of the output sequence, which indicates the number of rounds in BFS/Coloring/KCore.
