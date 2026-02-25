# ParSet: 基于集合语义的高性能并发数据结构
为基于顶点子集的图算法提供高性能、可组合、可并行的基础数据结构与操作接口。

唯一环境要求：Linux, GCC >= 9 (C++17)
支持图规模：2³² 顶点，2⁶⁴ 边

# 使用方法
## 编译与链接
在Makefile中-I链接到该库的inlude路径，并保证-I链接parlaylib的inlude路径，即可通过
```c++
#include <ParSet/ParSet.h>
```
开始使用ParSet的功能
使用样例参见benchmarks/BFS/ParSet/Makefile
## 使用
### 数据结构与构造函数
ParSet的基础数据结构是并行分层位图。提供了Frontier和Active两个类，其中Frontier是双缓冲结构，拥有两个位图实例，操作其中一个时可以向另一个插入，并通过advance()交换两者指针。构造函数中的参数n是集合最大大小，通常应为图的顶点数。Frontier默认全0，Active默认全1。
```c++
auto active = ParSet::Active(n); 
auto frontier = ParSet::Frontier(n);
```
以下说明只是常用功能示范，详细接口参见include/ParSet/vertex_parallel/frontier.h与include/ParSet/vertex_parallel/active.h
### 并发插入、删除与打包
ParSet是集合语义，多次插入同一个数值只会留存一份。
```c++
active.is_active(x);                  // x是否存在于active集合中
active.deactivate(x);                 // 从active集合中删除x
frontier.insert(x);                   // 向frontier的next中插入x
frontier.advance();                   // frontier内部双指针交换，并返回交换后
size_t size = frontier.pack_into(seq);// 将frontier中的元素打包到parlay::sequence
```
pack_into给予ParSet作为并行收集元素的容器的功能，
但它通常不是ParSet的推荐做法，因为ParSet支持直接并行操作集合。
### 并发操作
ParSet允许在不pack的情况下直接对集合元素作高效遍历，parallel_do是构建并行图算法的核心方法：
```c++
active.parallel_do([&](size_t s) { ... }); // 并行对每个active集合中的点s施加操作
frontier.parallel_do([&](size_t s) { ... }); // 并行对每个frontier集合中的点s施加操作，并清空frontier
```
默认情况下active.parallel_do操作后元素会保留，而frontier.parallel_do会清空frontier。
frontier.parallel_do过程中可以fronteir.insert()(插入到另一个实例)，此时当前实例被清空
然后frontier.advance交换指针后，已经清空的当前实例又会作为next接受新的insert。
因此frontier能够高效对齐轮次推进语义。
parallel_do携带模板参数true/false。如果不愿清空frontier，或者要清空active，也可以：
```c++
active.parallel_do<true>(...); // 并行对每个active集合中的点施加操作，清空
frontier.parallel_do<false>(...); // 并行对每个frontier集合中的点施加操作，不清空
```
### 并行规约
ParSet允许在不pack的情况下直接对集合元素作规约，且Work始终保持为当前集合元素数量。active和frontier中的接口功能一致。
```c++
active.reduce(
    [&](size_t s) { ... },               // 元素如何提供数值
    [&](uint64_t l, uint64_t r) { ... }, // 如何规约
);
```
注意ParSet只提供正整数集的规约操作，0被作为规约的哨兵值，因此reduce某种最小值时要避免元素提供数值0.
为了方便，提供了一些常见的规约操作
```c++
uint64_t res = active.reduce_max(sequence)// 对于active集合中的所有点s，计算sequence[s]中的最大值
uint64_t res = active.reduce_min(sequence)
uint64_t res = active.reduce_sum(sequence)
uint64_t res = active.reduce_vertex()     // 集合中元素数量
uint64_t res = active.reduce_edge(graph)  // 顶点集合的邻居总数量
```
### 并行规约且构建Augment Value
ParSet允许在规约时保存规约过程，以作他用。
pack的实现，实际就是先对1作和的规约，得到prefix_sum，再写入sequence。
```c++
// 规约最小值，并保留规约过程
active.reduce<true>(
    [&] (size_t i) { return array[i]; },
    [&] (uint64_t l, uint64_t r) { return (l == 0 || l > r) ? r : l; }
);
```
后续可以利用Augment Value快速提取最小/最大值。
```c++
// 利用规约过程快速提取等于最小值的元素
active.select(
    [&] (size_t i) { return array[i]; }, 
    [&] (size_t s) { ... } // 要对这些最小元素做的操作
);
```
为了方便，提供了操作所有最大值/最小值的函数
```c++
// 利用规约过程快速提取等于最小值的元素
active.reduce_and_select_max(array, lambda);
active.reduce_and_select_min(array, lambda);
```
### 并行随机弹出
用来支持无需pack的prefix doubling
```c++
active.pop(k, frontier, lambda); //随机弹出k个点到frontier
```

# 测试方法
进入 benchmark_utils 目录
复制配置文件：
```bash
cp example_of_config.sh config.sh
```
修改 config.sh：
将 BIN_DIR 设置为图数据存储路径
运行某算法，例如：
```bash
cd benchmarks/BFS/ParSet
./bench.sh
```
若只测试内置图：
```bash
./test.sh
```
HepPh_sym.bin 已内置，无需外部数据。