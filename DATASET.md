# Dataset

## Source

- Dataset: [Mylnikov Wi-Fi database dump](https://www.mylnikov.org/dump2019/wifi.csv)
- Snapshot: December 2019
- Columns used: `lat` and `lon`
- Full dump size: 2,256,786,828 bytes
- Full dump rows: 37,841,214 records with valid coordinates
- Full dump SHA-256: `566935ba428d9db6e361c3713f488cd918afa99aef196b7c4fb9d9e0e3f0b8b7`
- Small input: the first 1 MiB of the dump, truncated to complete CSV rows
- Small input size: 19,111 records with valid coordinates
- Small input SHA-256: `f67772f1df6191b68b2c7a13dee0effa2c6e9c4081e5ad1506ffb2141bee7639`

We keep the 1 MiB input for quick tests. It contains real records from the dump but is not geographically representative. It has 14,823 unique coordinate pairs across several countries, so we select one bounded area before building a graph.

The [Mylnikov source page](https://www.mylnikov.org/archives/1170) describes the dump as a compilation based on openBmap and OpenWifi/OpenWLANMap data. Anyone redistributing the raw records should follow the source licenses. We do not include the raw CSV or BSSIDs in this repository.

## Graph construction

- Create one vertex for each selected access-point record.
- Keep records with identical coordinates as separate vertices because they identify separate access points.
- Convert latitude and longitude to local meter coordinates using an equirectangular projection centered on the selected bounds.
- Connect 2 vertices when their projected distance is at most the chosen threshold.
- Use half-open geographic bounds to prevent duplicate records between adjacent regions.

We use one fixed threshold for every selected point. If we divide each projected coordinate by that threshold, the adjacency rule becomes a distance of at most 1. This makes the result a unit disk graph.

We use the local projection only for a bounded area. Applying it once to the worldwide dump would increase planar distance distortion as the selected region gets larger.

## Small input run

We selected `48 <= latitude < 49` and `2 <= longitude < 3` from the local sample and used a 100-meter threshold. The graph has 3,876 vertices and 14,120 edges. Geometric first-fit and smallest-last used 41 colors. Random-order first-fit used 42 colors with seed 6405. All 3 colorings passed the validity check. We chose the 100-meter value for this experiment because the dataset does not supply a distance threshold.

The selected records have 2,641 unique coordinate pairs. We keep all 3,876 access-point records as vertices.

## Full dataset run

We scanned all 37,841,214 records and selected `48 <= latitude < 49` and `2 <= longitude < 3`. At a 100-meter threshold, the graph has 1,383,531 vertices and 372,467,748 edges. Building the graph took 81.47 seconds and used 4.13 GiB of memory. The edge-list file uses 5.1 GiB.

The graph uses a bounded region because the local equirectangular projection does not preserve distances across the whole world. Scanning the full dump gives us all records from the selected region without treating worldwide coordinates as one plane.

| Algorithm | Colors | Coloring time | Valid |
|---|---:|---:|---:|
| Geometric first-fit | 1,387 | 5.48 seconds | Yes |
| Smallest-last | 1,310 | 12.59 seconds | Yes |
| Random-order first-fit (seed 6405) | 1,384 | 8.42 seconds | Yes |

The coloring times exclude graph loading and validation. Each complete coloring command took about 2 minutes because it loaded the 5.1 GiB edge list first.
