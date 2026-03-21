#!/usr/bin/env bash
set -euo pipefail

CXX_BASE="android-project/app/.cxx/Debug"
TARGET="${1:-}"

if [[ ! -d "${CXX_BASE}" ]]; then
  echo "[ERROR] Android CMake debug directory not generated: ${CXX_BASE}"
  echo "[INFO ] Run first: ./build-scripts/generate-project/android-unix.sh"
  exit 1
fi

if [[ -n "${TARGET}" ]]; then
  if [[ "${TARGET}" == "rc2d_example" ]]; then
    TARGET="main"
    echo "[INFO ] Android uses target 'main' instead of 'rc2d_example'."
  fi
  targets=("${TARGET}")
else
  targets=("rc2d" "main")
  echo "[INFO ] No target specified, building default Android development targets: rc2d + main"
fi

mapfile -t CMAKE_BUILD_DIRS < <(find "${CXX_BASE}" -type f -name CMakeCache.txt -exec dirname {} \; | sort -u)
if [[ ${#CMAKE_BUILD_DIRS[@]} -eq 0 ]]; then
  echo "[ERROR] No Android CMake build directories found under: ${CXX_BASE}"
  echo "[INFO ] Run first: ./build-scripts/generate-project/android-unix.sh"
  exit 1
fi

echo -e "\e[32m[INFO ] Rebuilding Debug configuration (Android Unix host)...\e[0m"
for dir in "${CMAKE_BUILD_DIRS[@]}"; do
  echo -e "\e[32m[INFO ] Using Android CMake dir: ${dir}\e[0m"
  for t in "${targets[@]}"; do
    echo -e "\e[32m[INFO ] Building target ${t} (Debug)...\e[0m"
    cmake --build "${dir}" --target "${t}" --parallel 8
  done
done

echo -e "\e[32m[OK   ] Debug build completed.\e[0m"
