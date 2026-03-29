# GraphIO

Provides graph loader, compatible with ParSet.

Requires: Linux (x86_64), GCC >= 10.

## Supported Format

Vertices: 2³² uint32_t
Edges: 2⁶⁴ uint64_t
Weight: Empty / int32_t / float
File: binary or pbbs format

## Usage

```cpp
Graph<Wgh>(const char *filename, int32_t r=0)
Graph.n
Graph.m
Graph.offsets[i]
Graph.edges[i].v
Graph.edges[i].w
```
