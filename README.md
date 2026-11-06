# Toggle Tree

A header-only C++ parallel algorithm library. It is based on a parallel data structure called `Toggle Tree`.

## 1. Benchmarking

### 1.1. Preparing Dataset

```bash
[toggle-tree/dataset]$ cp example_of_config.sh config.sh
```

Then configure your graph directory inside it.

Use the following commands to download and generate the graphs:

```bash
[toggle-tree/dataset]$ bash ./download_graphs.sh
[toggle-tree/dataset/generate_weight]$ bash ./generate.sh 16
```

### 1.2. Benchmarking

Test scripts are provided in `benchmarks/`.
For example:

```bash
[toggle-tree/benchmarks/graph/breadth_first_search/dist]$ bash ./test.sh
```

After running benchmarks, two `.csv` files will be generated.
`benchmark.csv` contains timing results. `verify.csv` provides a fast sanity check.

## 2. Usage

This is a header-only C++ library. You may add `include/` to the include path when compiling, or use `CMakeLists.txt`.

Standard: C++20

When `TOGGLE_TREE_WEAK_MEMORY` is defined, the library can be built on any architecture.
It is by default disabled on Linux (x86_64) with GCC/Clang, where Toggle Tree can then have an ideal theoretic bound under the BMI2 instruction set.

When `TOGGLE_TREE_USE_PARLAY` is defined, [parlaylib](https://github.com/cmuparlay/parlaylib) can be used as the scheduler,
Since different schedulers can not exist in the mean time, this option is for those who want to use Toggle Tree together with parlaylib primitives.
Note that parlaylib scheduler has a bug (see [parlaylib issue #99](https://github.com/cmuparlay/parlaylib/issues/99)).
You are recommended to fix it locally before usage.
Our default scheduler is a lightweight but reliable design: scheduler queue overflows or wraparounds do not cause it to fail.

## 3. Contents in the Library

### 3.1. parallel

#### 3.1.1. `parallel/parallel.h`

- `toggle::num_workers`
- `toggle::worker_id`
- `toggle::parallel_for`
- `toggle::parallel_do`

### 3.2. data_structures

#### 3.2.1. `data_structures/array.h`

- `toggle::Array`

#### 3.2.2. `data_structures/csr.h`

- `toggle::CSR`
  - `n`
  - `m`
  - `offset`
  - `out_offset`
  - `in_offset`
  - `edge`
  - `out_edge`
  - `in_edge`
  - `weighted_edge`
  - `weighted_out_edge`
  - `weighted_in_edge`

#### 3.2.3. `data_structures/index_set.h`

- `toggle::IndexSet`
  - `empty`
  - `contains`
  - `insert`
  - `remove`
  - `for_each`
  - `consume_each`
  - `for_range`
  - `consume_range`
  - `reduce`
  - `num_elements`
  - `sample`

#### 3.2.4. `data_structures/index_map.h`

- `toggle::IndexMap`
  - `empty`
  - `insert`
  - `repair`
  - `extract_min`
  - `extract_prefix_min`

#### 3.2.5. `data_structures/local_queue.h`

- `toggle::LocalQueue`
  - `pop_top`
  - `push_back`
  - `for_each`
  - `sequential_for_each`

### 3.3. primitives

- `toggle::reduce`
- `toggle::scan`
- `toggle::pack`
- `toggle::pack_into`
- `toggle::hash32`
- `toggle::hash64`
- `toggle::random_permutation`
- `toggle::sample_sort`

### 3.4. benchmark_utils

- `toggle::read_graph`
- `toggle::write_graph`
- `toggle::make_random_weight`

### 3.5. algorithms

- `toggle::breadth_first_search`
- `toggle::breadth_first_search_tree`
- `toggle::graph_coloring`
- `toggle::k_core_decomposition`
- `toggle::bellman_ford`
- `toggle::dijkstra`
