# Toggle Tree (ToT)

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## Benchmarking

Benchmarks in Toggle Tree are designed to produce bit-for-bit identical output with the GBBS/Hashbag baseline implementations.

### Environment Setup

No external dependencies are required for running ToT/Hashbag benchmarks.
They only rely on parlaylib, and we have vendored it for benchmarking.

Benchmarking GBBS baseline Requires bazel 7.7.1.
Bazel is only used to build GBBS baselines, and has nothing to do with Toggle Tree.

### Configuration

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

### Benchmarking

Each folder in `benchmarks/` is a seperate implementation, 
you can use `bash ./bench.sh` in them to run all the graphs, 
or `bash ./test.sh` for only one graph configured in `config.sh`.

Scripts for testing all the algorithms are also provided: 

```bash
[benchmark_utils/scripts]$ ./bench_all.sh
[benchmark_utils/scripts/BFS]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/Coloring]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/KCore]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/BellmanFord]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/BellmanFord]$ cat verify.csv && echo "------" && cat benchmark.csv
```

After running benchmarks, two `.csv` files will be generated.

`benchmark.csv` contains timing results.

`verify.csv` contains results of using the same hash function on both outputs of GBBS and Toggle Tree. This is intended as a fast sanity check, since writing and comparing full outputs can be impractical at scale. The hash function implementation is in `benchmark_utils/graph/verify.h`. Bitwise verification is provided in the next chapter.

These scripts does not include implementations in `benchmarks/_supplementary`. There are figure-related code, and sota implementations of algorithms.
Don't forget to cd into them and run `bash ./bench.sh`.

### Bitwise Verification

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
```

To select a certain graph with a certain algorithm, please edit the first few lines of `bitwise_verify.sh`.
