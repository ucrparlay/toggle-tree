# ParSet: Parallel Data Structures with Set Semantics

Provides high-performance, composable parallel data structures with set semantics for shared-memory graph algorithms.

Requires: Linux (x86_64), GCC >= 9 with C++17 support.
Supports graphs with up to 2³² vertices and 2⁶⁴ edges.

## Benchmarking
### output of benchmarking
After running an algorithm, three .csv files will be generated.
"benchmarks.csv" contains time,
"verify.csv" contains results of using the same hash function on both outputs of GBBS and ParSet. Comparing them verifies that GBBS and ParSet produce identical outputs (bit-for-bit). Code of hash function can be found in benchmark_utils/graph/verify.h.
"max.csv" contains the max result of the output sequence, which indicates the number of rounds in BFS/Coloring/KCore.
### test ParSet environment
For your convenience of reproducing experiments, no external dependencies are required for running benchmarks.
Since parlaylib is vendored and small graphs are saved for testing, those simple commands will directly run all the algorithms: 
```bash
cd benchmarks
./test_parset.sh
```
### test GBBS environment
Requires bazel 7.7.1
Bazel is used to build GBBS baselines. (has nothing to do with ParSet)
All the scripts are well prepared, as long as you get "bazel 7.7.1" when you run "bazel --version", 
those simple commands will directly run all the baselines: 
```bash
cd benchmarks
./test_gbbs.sh
```
### benchmarking
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