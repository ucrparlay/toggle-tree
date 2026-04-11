# GraphIO

Provides graph loader, compatible with Toggle Tree.

Requires: Linux (x86_64), GCC >= 10.

## Supported Format

Vertices: 2³² uint32_t
Edges: 2⁶⁴ uint64_t
Weight: Empty / int32_t / float
File: binary or pbbs format

## Usage

```cpp
graph_io::Graph<T> G(filepath)
G.n
G.m
G.offsets[i]
G.edges[i].idx
G.edges[i].wgh
G.in_offsets[i]
G.in_edges[i].idx
G.in_edges[i].wgh
```

```cpp
string graph_io::extract_graph_name(filepath)
bool graph_io::availability(result, proportion)
void graph_io::process_result(time, result, csvpath, row, column) 
```
