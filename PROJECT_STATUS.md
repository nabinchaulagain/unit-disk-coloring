# Project status

## Dataset

- We use the December 2019 Mylnikov Wi-Fi location dump.
- We use the full dump with 37,841,214 records. We keep a 19,111-record file for quick tests.
- We select a bounded area and convert its latitude and longitude to local meter coordinates.
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
- Our sample region has 3,876 vertices and 14,120 edges at a 100-meter threshold.
- Geometric first-fit and smallest-last used 41 colors. Random-order first-fit used 42 colors with seed 6405.
- DSATUR used 41 colors. All results passed the validity check.
- We scanned the full dump using the same region and threshold. The graph has 1,383,531 vertices and 372,467,748 edges.
- On the full run, geometric first-fit used 1,387 colors, smallest-last used 1,310 and random-order first-fit used 1,384 with seed 6405.
- All 3 full-run colorings passed the validity check.

## Remaining

- Choose the other geographic regions and distance thresholds for the experiments.
- Run the experiment matrix across those regions, thresholds and random seeds.
- Create comparison plots and finish the report and presentation.
