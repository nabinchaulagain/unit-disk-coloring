# Project status

## Dataset and graph generation

- We generate synthetic datasets for the project.
- We place points uniformly inside a square and treat each point as a vertex.
- We connect two vertices when their Euclidean distance is at most 1.
- We use fixed random seeds so each graph can be reproduced.

## Graph-generation algorithm

- We divide the plane into a grid.
- For each point, we inspect its cell and the neighboring cells.
- We add an edge when two points satisfy the unit-distance condition.
- This approach avoids checking every pair on sparse graphs.

## Assumptions

- All disks have the same radius.
- The normalized adjacency threshold is 1.
- The graph is simple, undirected, and static.
- Points at distance 1 are adjacent.
- The current dataset uses a uniform point distribution.
- The square size controls graph density.

## Coloring algorithm

- We use DSATUR.
- DSATUR selects the uncolored vertex with the largest number of distinct neighbor colors.
- It breaks ties using vertex degree.
- It assigns the smallest available color.
- DSATUR produces a valid coloring but may use more than the minimum number of colors.

## Current progress

- The uniform unit disk graph generator works.
- We checked the generator against an all-pairs method on small graphs.
- DSATUR works on the generated graphs.
- All current correctness and validity tests pass.

## Work remaining

- Add clustered unit disk graph datasets.
- Test several graph sizes, densities, and random seeds.
- Integrate geometric first-fit and smallest-last coloring.
- Run large-scale experiments and measure colors, runtime, and memory.
- Produce comparison plots.
- Complete the analysis, report, and presentation.
