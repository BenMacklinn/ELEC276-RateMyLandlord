#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/backend"

# Clean build
rm -rf build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j

# Free the port (macOS/Linux) - OPTIONAL
if lsof -ti :8080 >/dev/null 2>&1; then
  echo "Killing process on :8080..."
  kill -9 $(lsof -ti :8080)
fi


# Adding message that displays backend local port
echo "Starting backend on http://127.0.0.1:8080 ..."
exec ./rml_backend