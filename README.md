# Unit disk graph coloring

This project builds unit disk graphs from the Mylnikov Wi-Fi location dataset and colors them with DSATUR.

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

## Run DSATUR

```bash
./build/dsatur \
  --input graph.edgelist \
  --output measurement.csv \
  --colors assignments.csv
```

DSATUR selects the uncolored vertex with the highest saturation degree, which is the number of distinct colors used by its colored neighbors. It breaks ties by remaining degree and then vertex ID. The selected vertex gets the smallest available color.

The implementation uses an indexed max-heap for changing priorities, residual-degree and saturation arrays, a set of observed neighbor colors and timestamp marks for available colors. For `n` vertices and `m` edges, the expected running time is `O((n + m) log n)` and the working space is `O(n + m)` in the worst case. DSATUR returns a valid coloring but does not guarantee the minimum number of colors.

The measurement CSV contains the graph size, color count, coloring time and validity result. Graph construction and file parsing occur outside the coloring timer.

The synthetic generator remains available for small reference tests:

```bash
./build/generate_udg --vertices 100 --side 10 --threshold 1 --output test.edgelist
```

See [DATASET.md](DATASET.md) for the source, selection and modeling assumptions.
