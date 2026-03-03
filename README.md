# ParSet: Parallel Vertex Set

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## Benchmarking

Benchmarks in ParSet are designed to have bit-to-bit identical output with the provided GBBS baselines.

For the same input graph and deterministic tie-breaking, ParSet produces the exact same per-vertex output label array as GBBS (e.g., the same vertex receives the same BFS level, color id, k-core number, or distance). However, please note that their intermediate execution differs. This is expected: different abstraction levels naturally lead to different algorithmic implementations. ParSet is not a drop-in replacement for GBBS’s VertexSubset. Instead, ParSet uses a straightforward, natural implementation without aggressive, output-preserving optimizations intended to mimic GBBS.

### Output of Benchmarking

After running an algorithm, three .csv files will be generated.

"benchmark.csv" contains time.

"verify.csv" contains results of using the same hash function on both outputs of GBBS and ParSet. This is intended as a fast sanity check, since writing and comparing full outputs can be impractical at scale. Code of hash function can be found in benchmark_utils/graph/verify.h. Bitwise verification is also provided.

"max.csv" contains the max result of the output sequence, which indicates the number of rounds in BFS/Coloring/KCore.

### Testing ParSet Environment

For your convenience of reproducing experiments, no external dependencies are required for running benchmarks.

Since parlaylib is vendored and small graphs are saved for testing, those simple commands will directly run all the algorithms: 

```bash
[benchmark_utils/scripts]$ ./test_parset.sh
```

### Testing GBBS Environment

Requires bazel 7.7.1

Bazel is used to build GBBS baselines. (has nothing to do with ParSet)

All the scripts are well prepared, as long as you get "bazel 7.7.1" when you run "bazel --version", those simple commands will directly run all the baselines: 

```bash
[benchmark_utils/scripts]$ ./test_gbbs.sh
```

### Configure number of threads, directory of graphs and graphs to be benchmarked

To run all the benchmarks, 

Duplicate benchmark_utils/example_of_config.sh, rename it to "config.sh": 

```bash
[benchmark_utils/scripts]$ cp example_of_config.sh config.sh
```

Then configure number of threads, directory of graphs and graphs to be benchmarked inside it.

### Bitwise Verification

After successfully performed previous tests and configurations, you may test any benchmarked algorithm on any graph, to see if output of ParSet and GBBS are bit-to-bit identical.

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
```

To select a certain graph with a certain algorithm, please edit the first few lines of bitwise_verify.sh.

### Benchmarking

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
[benchmark_utils/scripts/BFS]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/Coloring]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/KCore]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/BellmanFord]$ cat verify.csv && echo "------" && cat benchmark.csv
```

## Usage

ParSet is a set, not a sequence. It allows you to consider directly how elements interact, and save the time of packing.

The source code in active.h and frontier.h are very clear and easy to understand its usage, so only a few explanations are necessary here.

### Active & Frontier
ParSet provides two classes: Active & Frontier.
Active contains one set, and its elements are not removed after visiting by default.
On the contrary, Frontier contains two sets, enabling insertion for the next round. 
Besides, Frontier's elements are by default removed after visiting.

### Insert & Remove
ParSet is a set, you can call insert on a same element for multiple times, and you will only see one element when you visit it.
Do NOT call insert and remove on a same set within a same round! That is NOT supported by ParSet, and no parallel graph algorithm is found that requires this kind of operation.
Frontier has two sets, so it is supported to call for_each on Frontier while calling insert_next, since they are operated on different sets.

### For Each & Pack
ParSet has good performance because it avoids packing. Pack is implemented only for special needs, while for_each is recommended to be used in common.

### Reduce & Select
You can provide a <True> flag, to let Reduction save its process on a augment tree.
Taking advantage of the augment tree, visiting elements has contains min/max value is supported.
It also enabled packing and edgemap, although both do not have better performance than for_each.
Make sure you call Reduce before calling Select. Otherwise there will be no augment tree.
