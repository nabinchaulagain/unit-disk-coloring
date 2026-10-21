#!/usr/bin/env bash
set -euo pipefail

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
dataset_path=${1:-"$project_dir/data/wifi_full_2019.csv"}
dataset_url="https://www.mylnikov.org/dump2019/wifi.csv"
expected_sha256="566935ba428d9db6e361c3713f488cd918afa99aef196b7c4fb9d9e0e3f0b8b7"
partial_path="${dataset_path}.part"

sha256_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
    return
  fi
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{print $1}'
    return
  fi
  echo "A SHA-256 tool is required. Install sha256sum or shasum." >&2
  exit 1
}

if ! command -v curl >/dev/null 2>&1; then
  echo "curl is required to download the dataset." >&2
  exit 1
fi

if [[ -f "$dataset_path" ]]; then
  actual_sha256=$(sha256_file "$dataset_path")
  if [[ "$actual_sha256" == "$expected_sha256" ]]; then
    echo "Dataset already exists and passed the SHA-256 check: $dataset_path"
    exit 0
  fi
  echo "The existing dataset failed the SHA-256 check: $dataset_path" >&2
  exit 1
fi

mkdir -p "$(dirname -- "$dataset_path")"
curl --fail --location --continue-at - --retry 3 --output "$partial_path" "$dataset_url"

actual_sha256=$(sha256_file "$partial_path")
if [[ "$actual_sha256" != "$expected_sha256" ]]; then
  echo "The downloaded dataset failed the SHA-256 check." >&2
  echo "Expected: $expected_sha256" >&2
  echo "Actual:   $actual_sha256" >&2
  exit 1
fi

mv "$partial_path" "$dataset_path"
echo "Dataset downloaded and verified: $dataset_path"
