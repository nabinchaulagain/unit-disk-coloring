# Unit disk graph coloring

A C++ project that builds unit disk graphs from the Mylnikov Wi-Fi location dataset and compares 3 greedy coloring orders.

## Graph model

A vertex represents one Wi-Fi access-point record. One real Wi-Fi network can have multiple access points, so a vertex is closer to a recorded radio or BSSID than a household or business network. We remove BSSIDs from the generated files.

An edge means 2 recorded access points are within the chosen distance threshold. At a 100-meter threshold, we treat access points no more than 100 meters apart as a possible wireless conflict and assign them different colors.

The dataset does not include signal strength, transmit power, walls, channels or traffic. An edge does not prove that the access points communicate, have overlapping coverage or belong to the same network. It represents physical proximity under the unit disk assumption. The output colors are graph-color labels, not observed Wi-Fi channel assignments.

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

The command below selects records from the half-open area `48 <= latitude < 49` and `2 <= longitude < 3`. The importer maps the coordinates to a local plane in meters and connects every pair at most 100 meters apart. We choose the threshold for each experiment.

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

The importer creates one vertex for each dataset row and keeps co-located access points separate. The output uses sequential vertex IDs and does not contain BSSIDs.

The importer uses an equirectangular projection centered on the selected area. A spatial grid limits each distance check to the current cell and its 8 neighboring cells. We create a simple undirected graph and connect points whose distance is at most the threshold. Dividing each projected coordinate by the fixed threshold gives the same graph with a unit distance limit.

## Run the coloring algorithms

Geometric first-fit processes vertices from left to right. For equal x-coordinates, it uses the y-coordinate and then the vertex ID.

```bash
./build/color_graph \
  --sorted-input \
  --algorithm geometric-first-fit \
  --input graph.edgelist \
  --points points.csv \
  --output geometric.csv
```

Smallest-last removes a vertex with minimum current degree until no vertices remain. It then applies first-fit in reverse removal order.

```bash
./build/color_graph \
  --sorted-input \
  --algorithm smallest-last \
  --input graph.edgelist \
  --output smallest-last.csv
```

Random-order first-fit shuffles the vertices using a recorded seed and then colors them.

```bash
./build/color_graph \
  --sorted-input \
  --algorithm random-first-fit \
  --seed 6405 \
  --input graph.edgelist \
  --output random.csv
```

Each method assigns the smallest color that none of the colored neighbors use. After we build the graph, geometric first-fit takes `O(n log n + m)` time. Smallest-last uses degree buckets and takes `O(n + m)` time. Random-order first-fit also takes `O(n + m)` time. Each method uses `O(n + m)` total space including the graph.

The command also supports DSATUR:

```bash
./build/color_graph \
  --sorted-input \
  --algorithm dsatur \
  --input graph.edgelist \
  --output dsatur.csv
```

DSATUR selects the uncolored vertex with the highest saturation degree. It breaks ties using the remaining degree and then the vertex ID. The indexed max-heap implementation has an expected running time of `O((n + m) log n)`.

Each measurement CSV contains the algorithm, graph size, color count, coloring time, validity result and random seed when used. The coloring timer excludes graph construction and file parsing.

The importer writes edges in sorted order. The `--sorted-input` option loads these files without keeping a second copy of every edge in memory, which is needed for the full dataset run.

The synthetic generator remains available for small reference tests:

```bash
./build/generate_udg --vertices 100 --side 10 --threshold 1 --output test.edgelist
```

See [DATASET.md](DATASET.md) for the source, selection and modeling assumptions.
