#!/usr/bin/env bash
set -euo pipefail

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
dataset_path=${1:-"$project_dir/data/wifi_full_2019.csv"}
result_dir=${2:-"$project_dir/results/full-region"}
build_dir="$project_dir/build"
graph_path="$result_dir/graph.edgelist"
points_path="$result_dir/points.csv"

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is required to build the project." >&2
  exit 1
fi

if [[ ! -f "$dataset_path" ]]; then
  echo "Dataset not found: $dataset_path" >&2
  echo "Run ./scripts/download_dataset.sh first." >&2
  exit 1
fi

mkdir -p "$result_dir"

cmake -S "$project_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure

"$build_dir/import_mylnikov" \
  --input "$dataset_path" \
  --min-lat 48 \
  --max-lat 49 \
  --min-lon 2 \
  --max-lon 3 \
  --threshold-meters 100 \
  --output "$graph_path" \
  --points "$points_path" \
  --metrics "$result_dir/generation.csv"

"$build_dir/color_graph" \
  --sorted-input \
  --algorithm geometric-first-fit \
  --input "$graph_path" \
  --points "$points_path" \
  --output "$result_dir/geometric.csv"

"$build_dir/color_graph" \
  --sorted-input \
  --algorithm smallest-last \
  --input "$graph_path" \
  --output "$result_dir/smallest-last.csv"

"$build_dir/color_graph" \
  --sorted-input \
  --algorithm random-first-fit \
  --seed 6405 \
  --input "$graph_path" \
  --output "$result_dir/random.csv"

echo "Results written to: $result_dir"
