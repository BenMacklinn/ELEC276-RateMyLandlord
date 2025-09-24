#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/backend"

# Clean build
rm -rf build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j


# Adding message that displays backend local port
echo "Starting backend on http://127.0.0.1:8080 ..."
exec ./rml_backend