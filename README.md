# DSATUR coloring

This directory implements Nabin Chaulagain's part of the COT 6405 unit-disk
coloring project: the DSATUR heuristic and efficient data structures for its
dynamically changing priorities.

The coloring code operates on an adjacency list, so it can be connected to the
team's shared spatial-grid graph generator without changing the algorithm. The
command-line program currently reads a simple undirected edge list:

```text
n m
u_1 v_1
...
u_m v_m
```

Vertex IDs are zero-based. Blank lines and lines beginning with `#` are
ignored. Self-loops and out-of-range endpoints are rejected, while duplicate
undirected edges are removed.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run DSATUR

Generate a uniform unit disk graph first. The command samples points from a
square and joins each pair whose Euclidean distance is at most the threshold.
The default threshold is 1, matching the submitted proposal.

```bash
./build/generate_udg \
  --vertices 10000 \
  --side 100 \
  --threshold 1 \
  --seed 6405 \
  --output graph.edgelist \
  --points points.csv \
  --metrics generation.csv
```

The generator uses a spatial grid with cells equal to the distance threshold.
It checks candidates from the point's cell and eight neighboring cells. Small
test instances are compared with an all-pairs implementation.

Color the generated graph:

```bash
./build/dsatur \
  --input graph.edgelist \
  --output measurement.csv \
  --colors assignments.csv
```

The measurement CSV records the graph size, number of colors, coloring time,
and validity check. The optional assignments CSV records each zero-based vertex
ID and its one-based color. Graph construction and input parsing occur before
the timed region, which allows fair comparison against the other coloring
methods.

For peak resident memory on Linux, wrap the command with `/usr/bin/time -v` and
record `Maximum resident set size` along with compiler, optimization flags,
CPU, and RAM.

## Algorithm and data structures

DSATUR repeatedly selects an uncolored vertex with the largest *saturation
degree*: the number of distinct colors already present among its neighbors.
Ties are broken by the vertex's degree in the remaining uncolored graph, then
by vertex ID so runs are deterministic. The selected vertex receives the
smallest color not used by a colored neighbor.

The implementation uses:

- an indexed max-heap with one entry per uncolored vertex, allowing an updated
  priority to move in `O(log n)` time without stale duplicate heap entries;
- residual degree and saturation arrays using `O(n)` space;
- a hash set of `(vertex, neighbor-color)` pairs, so a saturation value rises
  only when a genuinely new neighboring color appears; and
- timestamp marks to find the smallest available color without clearing an
  `O(n)` array for every vertex.

For `n` vertices and `m` edges, initialization takes `O(n + m)` time. At most
one heap update is performed for each endpoint when an edge becomes incident
to a newly colored vertex, giving `O((n + m) log n)` expected time overall.
The working storage is `O(n + m)` in the worst case, in addition to the shared
adjacency lists. The hash-table operations have expected constant cost.

The result is proper because a selected vertex is assigned a color absent from
all already colored neighbors. When the other endpoint of any edge is colored
later, it makes the same check, so the endpoints differ at termination. DSATUR
is a heuristic: validity is guaranteed, but a minimum-color result is not.
