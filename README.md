# Unit disk graph coloring

This project builds unit disk graphs from the Mylnikov Wi-Fi location dataset and compares 3 greedy coloring orders.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build -j
```

```bash
ctest --test-dir build --output-on-failure
```

## Build a graph from the dataset

Download the [Mylnikov 2019 Wi-Fi dump](https://www.mylnikov.org/dump2019/wifi.csv) or use the local sample in `datasets/mylnikov/wifi_sample_1MiB.csv`.

The command below selects records in the half-open area `48 <= latitude < 49` and `2 <= longitude < 3`. It maps the coordinates to a local plane measured in meters, then connects every pair at most 100 meters apart. The threshold is an experiment parameter.

```bash
./build/import_mylnikov \
  --input ../../../datasets/mylnikov/wifi_sample_1MiB.csv \
  --min-lat 48 \
  --max-lat 49 \
  --min-lon 2 \
  --max-lon 3 \
  --threshold-meters 100 \
  --output graph.edgelist \
  --points points.csv \
  --metrics generation.csv
```

Each dataset row becomes one vertex. This keeps co-located access points separate. The generated files use sequential vertex IDs and do not contain BSSIDs.

The importer uses an equirectangular projection centered on the selected area. A spatial grid restricts distance checks to the current cell and its eight neighboring cells. The graph is simple and undirected, and points exactly at the threshold are adjacent. Dividing every projected coordinate by the fixed threshold gives the equivalent unit-distance representation.

## Run the coloring algorithms

Geometric first-fit processes vertices from left to right. Equal x-coordinates are ordered by y-coordinate and then vertex ID.

```bash
./build/color_graph \
  --algorithm geometric-first-fit \
  --input graph.edgelist \
  --points points.csv \
  --output geometric.csv
```

Smallest-last repeatedly removes a vertex with minimum current degree. It reverses the removal order before applying first-fit.

```bash
./build/color_graph \
  --algorithm smallest-last \
  --input graph.edgelist \
  --output smallest-last.csv
```

Random-order first-fit shuffles the vertices with a recorded seed before coloring them.

```bash
./build/color_graph \
  --algorithm random-first-fit \
  --seed 6405 \
  --input graph.edgelist \
  --output random.csv
```

Each method assigns the smallest color not used by an already colored neighbor. Geometric first-fit takes `O(n log n + m)` time after graph construction. Smallest-last uses degree buckets and takes `O(n + m)` time. Random-order first-fit also takes `O(n + m)` time. All 3 use `O(n + m)` total space with the graph.

The comparison command also supports DSATUR:

```bash
./build/color_graph \
  --algorithm dsatur \
  --input graph.edgelist \
  --output dsatur.csv
```

DSATUR selects the uncolored vertex with the highest saturation degree. It breaks ties by remaining degree and then vertex ID. Its expected running time is `O((n + m) log n)` with the indexed max-heap implementation.

Each measurement CSV contains the algorithm, graph size, color count, coloring time, validity result and random seed when used. Graph construction and file parsing occur outside the coloring timer.

The synthetic generator remains available for small reference tests:

```bash
./build/generate_udg --vertices 100 --side 10 --threshold 1 --output test.edgelist
```

See [DATASET.md](DATASET.md) for the source, selection and modeling assumptions.
