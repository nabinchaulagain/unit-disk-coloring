# Project status

## Dataset

- We use the December 2019 Mylnikov Wi-Fi location dump.
- Our local file is a 19,111-record sample from the start of the dump, so it is not geographically representative.
- We select one bounded area and convert its latitude and longitude to local meter coordinates.
- Each access-point record becomes one vertex. We connect 2 vertices when their distance is within the chosen threshold.

## Algorithms

- A spatial grid generates the unit disk graph without comparing every pair of points.
- Geometric first-fit orders vertices from left to right.
- Smallest-last removes minimum-degree vertices and colors them in reverse removal order.
- Random-order first-fit shuffles vertices with a recorded seed.
- DSATUR is available as an additional comparison.
- Each coloring method assigns the smallest color unused by an already colored neighbor.

## Progress

- The real-data importer, geographic projection and graph generator work.
- All 3 proposed coloring algorithms and the validity check work.
- Unit tests compare the grid generator with an all-pairs implementation on small graphs.
- A sample region produced 3,876 vertices and 14,120 edges at a 100-meter threshold.
- Geometric first-fit and smallest-last used 41 colors. Random-order first-fit used 42 colors with seed 6405.
- DSATUR also used 41 colors. Every result passed the validity check.

## Remaining

- Agree on the geographic regions and distance thresholds for the experiments.
- Run the full experiment matrix and record colors, time and memory.
- Create comparison plots and finish the report and presentation.
