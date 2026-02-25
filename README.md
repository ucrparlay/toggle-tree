# ParSet: High-Performance Concurrent Data Structures with Set Semantics

Provides high-performance, composable, and parallel foundational data structures and operation interfaces for graph algorithms based on vertex subsets.

Requires Linux, GCC >= 9 (C++17)
Supports graphs with up to 2³² vertices and 2⁶⁴ edges.

## Benchmarking
### test ParSet environment
For your convenience of reproducing expeiments, this library does not rely on any outer repository to run benchmarks.
Since parlaylib is vendored and small graphs are saved for testing, those simple commands will directly run all the algorithms: 
```bash
cd benchmarks
./test_parset.sh
```
### test GBBS environment
Requires bazel 7.7.1
Bazel is used to build GBBS baselines. (has nothing to do with ParSet)
Al the scripts are well prepared, as long as you get "bazel 7.7.1" when you run "bazel --version", 
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