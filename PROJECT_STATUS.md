# Project status

## Dataset

- We use the December 2019 Mylnikov Wi-Fi location dump.
- Our local file is a 19,111-record sample from the start of the dump, so it is not geographically representative.
- We select one bounded area and convert its latitude and longitude to local meter coordinates.
- Each access-point record becomes one vertex. We connect 2 vertices when their distance is within the chosen threshold.

## Algorithms

- A spatial grid generates the unit disk graph without comparing every pair of points.
- DSATUR selects the vertex with the most distinct neighbor colors, breaks ties by degree and assigns the smallest available color.
- DSATUR guarantees a valid coloring, but it may use more than the minimum number of colors.

## Progress

- The real-data importer, geographic projection and graph generator work.
- The DSATUR implementation and validity check work.
- Unit tests compare the grid generator with an all-pairs implementation on small graphs.
- A sample region produced 3,876 vertices and 14,120 edges at a 100-meter threshold. DSATUR used 41 colors and returned a valid coloring.

## Remaining

- Agree on the geographic regions and distance thresholds for the experiments.
- Add the other coloring algorithms used by the group.
- Run the full experiment matrix and record colors, time and memory.
- Create comparison plots and finish the report and presentation.
