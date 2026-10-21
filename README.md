# Unit disk graph coloring

A C++ project that builds unit disk graphs from the Mylnikov Wi-Fi location dataset and compares 3 greedy coloring orders.

## Run on another device

Install Git, CMake 3.16 or newer, a C++17 compiler, Bash and `curl`. The full run needs about 12 GiB of free disk space and 8 GiB of available memory.

```bash
git clone https://github.com/nabinchaulagain/unit-disk-coloring.git
```

```bash
cd unit-disk-coloring
```

Download the 2.1 GiB dataset and verify its SHA-256 checksum.

```bash
./scripts/download_dataset.sh
```

Build the project, run the tests, generate the reported graph and run the 3 required coloring algorithms.

```bash
./scripts/run_full_region.sh
```

The script scans the full dataset and builds the graph for `48 <= latitude < 49` and `2 <= longitude < 3` using a 100-meter threshold. It writes generated files to `results/full-region/`.

Claude Code reads `CLAUDE.md`, which imports the project instructions from `AGENTS.md`. Start Claude Code from the repository root and ask it to run the full region experiment.

```bash
claude
```

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

Download the [Mylnikov 2019 Wi-Fi dump](https://www.mylnikov.org/dump2019/wifi.csv) with `./scripts/download_dataset.sh`. The reported experiment uses the full dump stored as `data/wifi_full_2019.csv`. The smaller `wifi_sample_1MiB.csv` file is only for quick checks.

The command below selects records from the half-open area `48 <= latitude < 49` and `2 <= longitude < 3`. The importer maps the coordinates to a local plane in meters and connects every pair at most 100 meters apart. We choose the threshold for each experiment.

```bash
./build/import_mylnikov \
  --input data/wifi_full_2019.csv \
  --min-lat 48 \
  --max-lat 49 \
  --min-lon 2 \
  --max-lon 3 \
  --threshold-meters 100 \
  --output graph.edgelist \
  --points points.csv \
  --metrics generation.csv
```

## Unit disk graph generation

1. Read the source CSV and keep records with valid coordinates inside the selected geographic bounds.
2. Create one vertex for every selected access-point record. Records at the same coordinates remain separate vertices.
3. Convert latitude and longitude to local `x` and `y` coordinates in meters using an equirectangular projection centered on the selected area.
4. Divide the area into square grid cells whose side length equals the distance threshold.
5. For each vertex, check vertices in its cell and the 8 neighboring cells. Add an undirected edge when the exact Euclidean distance is at most the threshold.
6. Write the graph, projected coordinates and generation measurements to their output files.

We do not remove a vertex because it has no other vertex within the threshold. It remains in the graph as an isolated vertex with degree 0. We only skip invalid records and records outside the selected geographic bounds.

For the current experiment, the distance threshold is 100 meters. If we divide every projected coordinate by 100, the same edge rule becomes a distance of at most 1 unit. The 1 unit is a normalized distance, not 1 meter. Using one fixed threshold for every vertex makes this a unit disk graph.

The spatial grid reduces the number of distance calculations. It does not change the graph because we still apply the exact distance test before adding each edge.

## CSV files

- `wifi_full_2019.csv` is the full raw Mylnikov dataset used for the reported experiment. We read its latitude and longitude values but do not commit this file because it is about 2.1 GiB.
- `wifi_sample_1MiB.csv` contains the first 1 MiB of the same dataset and is only used for quick checks. Results from this file are not part of the reported full-dataset experiment.
- `points.csv` maps each selected vertex ID to its original latitude and longitude and its projected `x` and `y` coordinates. Geometric first-fit uses this file to order the vertices.
- `generation.csv` contains one row of graph-generation measurements, including the number of input rows, vertices, edges, threshold and generation time.
- `geometric.csv`, `smallest-last.csv`, `random.csv` and `dsatur.csv` contain the result from each coloring run. These files include the color count, coloring time and validity result.

The rows below use a 2-vertex example to show each format. `AA` and `BB` are shortened BSSID values.

`wifi_full_2019.csv` or `wifi_sample_1MiB.csv`:

```csv
id,bssid,lat,lon,updated,data
1,AA,48.500000,2.500000,0,0
2,BB,48.500000,2.501000,0,0
```

`points.csv`:

```csv
vertex,latitude,longitude,x_meters,y_meters
0,48.5,2.5,0.0,0.0
1,48.5,2.501,74.0,0.0
```

The 2 vertices are about 74 meters apart, so a 100-meter threshold creates one edge. The graph edge-list file is not a CSV. Its first row stores the number of vertices and edges, and the remaining rows store the endpoint IDs:

```text
2 1
0 1
```

`generation.csv`:

```csv
dataset,rows_read,invalid_rows,vertices,edges,threshold_meters,min_latitude,max_latitude,min_longitude,max_longitude,reference_latitude,reference_longitude,generation_ms
mylnikov_wifi,2,0,2,1,100,48,49,2,3,48.5,2.5,0.150000
```

A coloring-result CSV:

```csv
algorithm,vertices,edges,colors,coloring_ms,valid,seed
geometric-first-fit,2,1,2,0.010000,1,
random-first-fit,2,1,2,0.009000,1,6405
```

The generation and coloring times in these sample rows only demonstrate the file format. Each run writes its measured time.

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
