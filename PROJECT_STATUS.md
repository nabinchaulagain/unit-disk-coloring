# Project status

## Dataset

- We use the December 2019 Mylnikov Wi-Fi location dump.
- We use the full dump with 37,841,214 records.
- We select the Paris region using `48 <= latitude < 49` and `2 <= longitude < 3`, then convert its coordinates to local meters.
- Each access-point record becomes one vertex. We connect 2 vertices when their distance is within the chosen threshold.

## Algorithms

- We use a spatial grid to generate the unit disk graph without comparing every pair of points.
- Geometric first-fit orders vertices from left to right.
- Smallest-last removes minimum-degree vertices and colors them in reverse removal order.
- Random-order first-fit shuffles vertices with a recorded seed.
- We also use DSATUR as an additional comparison.
- Each coloring method assigns the smallest color unused by an already colored neighbor.

## Progress

- We finished the real-data importer, geographic projection and graph generator.
- We implemented all 3 proposed coloring algorithms and the validity check.
- We compare the grid generator with an all-pairs implementation in unit tests on small graphs.
- We scanned the full dump and generated a graph with 1,383,531 vertices and 372,467,748 edges at a 100-meter threshold.
- Graph generation took 81.47 seconds and reached 4.13 GiB peak memory.

| Algorithm | Colors | Coloring time | Peak memory |
|---|---:|---:|---:|
| Geometric first-fit | 1,387 | 5.48 seconds | 4.18 GiB |
| Smallest-last | 1,310 | 12.59 seconds | 4.18 GiB |
| Random-order first-fit (seed 6405) | 1,384 | 8.42 seconds | 4.16 GiB |

- The coloring times exclude graph loading and validation. Peak memory covers the full command, including graph loading.
- All 3 full-run colorings passed the validity check.

## Remaining

- Choose the other geographic regions and distance thresholds for the experiments.
- Run the experiment matrix across those regions, thresholds and random seeds.
- Create comparison plots and finish the report and presentation.
