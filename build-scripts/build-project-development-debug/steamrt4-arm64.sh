#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build/steamrt4/arm64/Debug"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
TARGET="${1:-}"

if [[ ! -f "${CACHE_FILE}" ]]; then
  echo "[ERROR] Build directory not generated: ${BUILD_DIR}"
  echo "[INFO ] Run first: ./build-scripts/generate-project/steamrt4-arm64.sh"
  exit 1
fi

echo -e "\e[32m[INFO ] Rebuilding Debug configuration (SteamRT4 arm64)...\e[0m"
if [[ -n "${TARGET}" ]]; then
  targets=("${TARGET}")
else
  targets=("rc2d" "rc2d_example")
  echo -e "\e[32m[INFO ] No target specified, building default development targets...\e[0m"
fi

for t in "${targets[@]}"; do
  echo -e "\e[32m[INFO ] Building target ${t} (Debug)...\e[0m"
  cmake --build "${BUILD_DIR}" --target "${t}" --parallel 8
done

echo -e "\e[32m[OK   ] Debug build completed.\e[0m"
