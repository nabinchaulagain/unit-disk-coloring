# Dataset

## Source

- Dataset: [Mylnikov Wi-Fi database dump](https://www.mylnikov.org/dump2019/wifi.csv)
- Snapshot: December 2019
- Columns used: `lat` and `lon`
- Local input: the first 1 MiB of the dump, truncated to complete CSV rows
- Local sample size: 19,111 records with valid coordinates
- Local sample SHA-256: `f67772f1df6191b68b2c7a13dee0effa2c6e9c4081e5ad1506ffb2141bee7639`

The local sample contains real records from the dump, but the first 1 MiB does not represent the full dataset geographically. It contains 14,823 unique coordinate pairs and spans several countries. We therefore select one bounded area before constructing a graph.

The [Mylnikov source page](https://www.mylnikov.org/archives/1170) describes the dump as a compilation based on openBmap and OpenWifi/OpenWLANMap data. Follow the source licenses when redistributing the raw records. This repository does not include the raw CSV or BSSIDs.

## Graph construction

- Use each selected access-point record as one vertex.
- Keep records with identical coordinates as separate vertices because they identify separate access points.
- Project latitude and longitude to local meter coordinates with an equirectangular projection centered on the selected bounds.
- Connect 2 vertices when their projected distance is at most the chosen threshold.
- Use half-open geographic bounds to prevent duplicate records when adjacent regions are processed.

One fixed threshold applies to every selected point. Dividing each projected coordinate by that threshold normalizes the adjacency rule to distance at most 1, so the resulting graph is a unit disk graph.

The local projection works for a bounded area. It should not be used once for the entire worldwide dump because planar distance distortion grows with the size of the selected region.

## Verified sample run

We selected `48 <= latitude < 49` and `2 <= longitude < 3` from the local sample and used a 100-meter threshold. This produced 3,876 vertices and 14,120 edges. DSATUR used 41 colors and returned a valid coloring. The 100-meter value is a chosen experiment setting, not a distance supplied by the dataset.

The selected records contain 2,641 unique coordinate pairs. The graph keeps all 3,876 access-point records as vertices.
