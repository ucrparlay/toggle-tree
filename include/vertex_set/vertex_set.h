#pragma once
#include "parlay/parallel.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"
#include "graph/graph.h"
#include "graph/edge_map_blocked.h"
#include "bitmap/linear_bitmap.h"
#include "bitmap/edge_bitmap.h"
#include "hashbag/hashbag.h"
#include "counter/tree_counter.h"
#if defined(STR)
    #include "counter/stripped_nonzero_counter.h"
#elif defined(INFO)
    #include "counter/info_counter.h"
#else
    #include "counter/deterministic_counter.h"
#endif
#if defined(HASH)
    #include "bitmap/frontier_hashbag.h"
#else
    #include "bitmap/frontier_bitmap.h"
#endif
#if defined(PACK)
    #include "bitmap/active_pack.h"
#else
    #include "bitmap/active_bitmap.h"
#endif