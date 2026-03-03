# ParSet: Parallel Vertex Set

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 10.

Supports graphs with up to 2³⁶ vertices and 2⁶⁴ edges.

## Benchmarking

Benchmarks in ParSet are designed to be label-consistent with the provided GBBS baselines: for the same input graph and deterministic tie-breaking, ParSet produces the exact same per-vertex output label array as GBBS (e.g., the same vertex receives the same BFS level, color id, k-core number, or distance).

### Output of Benchmarking

After running an algorithm, three .csv files will be generated.

"benchmark.csv" contains time.

"verify.csv" contains results of using the same hash function on both outputs of GBBS and ParSet. This is intended as a fast sanity check; it does not constitute a full element-wise proof on large graphs, since writing and comparing full outputs can be impractical at scale. Code of hash function can be found in benchmark_utils/graph/verify.h.

"max.csv" contains the max result of the output sequence, which indicates the number of rounds in BFS/Coloring/KCore.

### Testing ParSet Environment

For your convenience of reproducing experiments, no external dependencies are required for running benchmarks.

Since parlaylib is vendored and small graphs are saved for testing, those simple commands will directly run all the algorithms: 

```bash
cd benchmarks
./test_parset.sh
```

### Testing GBBS Environment

Requires bazel 7.7.1

Bazel is used to build GBBS baselines. (has nothing to do with ParSet)

All the scripts are well prepared, as long as you get "bazel 7.7.1" when you run "bazel --version", those simple commands will directly run all the baselines: 

```bash
cd benchmarks
./test_gbbs.sh
```

### Benchmarking

To run all the benchmarks, 

Duplicate benchmark_utils/example_of_config.sh, rename it to "config.sh": 

```bash
cp benchmark_utils/example_of_config.sh benchmark_utils/config.sh
```

Then configure your directory that contains graphs and configure what you are going to run inside it.

After that, those simple commands will reproduce all the table reported in the paper:  

```bash
cd benchmarks
./bench_all.sh
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
