# ParSet: Parallel Vertex Set

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## Benchmarking

Benchmarks in ParSet are designed to produce bit-for-bit identical output with the GBBS baseline implementations.

### Testing ParSet Environment

To make experiments easy to reproduce, no external dependencies are required for running benchmarks.

Since parlaylib is vendored and small graphs are included, this command will directly compile and test all the algorithms: 

```bash
[benchmark_utils/scripts]$ ./test_parset.sh
```

### Testing GBBS Environment

Requires bazel 7.7.1.

Bazel is used to build GBBS baselines. (has nothing to do with ParSet)

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

After successfully performed previous tests and configurations, you may test any benchmarked algorithm on any graph, to see if output of ParSet and GBBS are bit-for-bit identical.

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
```

To select a certain graph with a certain algorithm, please edit the first few lines of `bitwise_verify.sh`.

For the same input graph and deterministic tie-breaking, ParSet produces the exact same per-vertex output label array as GBBS (e.g., the same vertex receives the same BFS level, color id, k-core number, or distance). However, please note that their intermediate execution differs. This is expected: different abstraction levels naturally lead to different algorithmic implementations. ParSet is not a drop-in replacement for GBBS’s VertexSubset. Instead, ParSet uses a straightforward, natural implementation without aggressive, output-preserving optimizations intended to mimic GBBS.

### Benchmarking

```bash
[benchmark_utils/scripts]$ ./bitwise_verify.sh
[benchmark_utils/scripts/BFS]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/Coloring]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/KCore]$ cat verify.csv && echo "------" && cat benchmark.csv
[benchmark_utils/scripts/BellmanFord]$ cat verify.csv && echo "------" && cat benchmark.csv
```

After running benchmarks, three `.csv` files will be generated.

`benchmark.csv` contains timing results.

`verify.csv` contains results of using the same hash function on both outputs of GBBS and ParSet. This is intended as a fast sanity check, since writing and comparing full outputs can be impractical at scale. The hash function implementation is in `benchmark_utils/graph/verify.h`. Bitwise verification is also provided elsewhere.

`max.csv` contains the max result of the output sequence, which indicates the number of rounds in BFS/Coloring/KCore.

## Usage

ParSet is a set, not a sequence. It lets you reason about element membership directly and avoid the cost of packing.

The source code in `active.h` and `frontier.h` are clear and make usage easy to understand, so only a few explanations are provided here.

### Linking

ParSet does not include a runtime; you must link parlaylib yourself.
The vendored parlaylib in `benchmark_utils` is only used for benchmarking.
ParSet is known to work with parlaylib commit: 51017699dcc421f80479cdb238d3092233ad0d26.
An example is shown in:
```bash
[benchmarks/BFS/ParSet]$ cat Makefile
```

### Active & Frontier
ParSet provides two classes: Active & Frontier.
Active contains one set, and its elements are not removed after visiting by default.
On the contrary, Frontier contains two sets, enabling insertion for the next round. 
Besides, Frontier's elements are removed by default after visiting.

### Insert & Remove
ParSet is a set, you can nsert the same element multiple times, and you will only see one element when you visit it.
Do NOT call insert and remove on a same set within the same round! That is NOT supported by ParSet, and no parallel graph algorithm is found that requires this kind of operation.
Frontier has two sets, so it is supported to call for_each on Frontier while calling insert_next, since they are operated on different sets.

### For Each & Pack
ParSet has good performance because it avoids packing. Pack is implemented only for special needs, while for_each is recommended to be used in common.

### Reduce & Select
You can provide a <True> flag, to let Reduction save its process on a augment tree.
Taking advantage of the augment tree, visiting elements has contains min/max value is supported.
It also enables packing and edgemap, although both do not have better performance than for_each.
Make sure Reduce is called before Select. Otherwise there will be no augment tree.
