#!/usr/bin/env bash
# Canonical version, flags, dependency discovery and license exclusions live in CMake.
set -euo pipefail
cd "$(dirname "$0")"
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
cpack --config build/linux-release/CPackConfig.cmake -G DEB
