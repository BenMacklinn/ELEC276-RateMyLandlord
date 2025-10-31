#!/usr/bin/env bash
set -euo pipefail

# Gmail SMTP configuration (defaults can be overridden via environment variables)
export SMTP_HOST="${SMTP_HOST:-smtp.gmail.com}"
export SMTP_PORT="${SMTP_PORT:-587}"
export SMTP_USERNAME="${SMTP_USERNAME:-elec376group10sender@gmail.com}"
export SMTP_PASSWORD="${SMTP_PASSWORD:-gpcpxlaflxknxzeb}"
export SMTP_FROM="${SMTP_FROM:-$SMTP_USERNAME}"
export SMTP_FROM_NAME="${SMTP_FROM_NAME:-RateMyLandlord}"

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/backend"

# Clean build
rm -rf build
mkdir -p build && cd build
if [ ! -f CMakeCache.txt ]; then
  cmake .. \
    -DMYSQL_INCLUDE_DIR=/usr/include/mysql \
    -DMYSQL_LIBRARY_DIR=/lib/x86_64-linux-gnu \
    -DMYSQL_LIBRARIES=/lib/x86_64-linux-gnu/libmysqlclient.so \
    -DCMAKE_BUILD_TYPE=Release \
    -Wno-dev
fi

cmake --build . -j

# Free the port (macOS/Linux) - OPTIONAL
if lsof -ti :8080 >/dev/null 2>&1; then
  echo "Killing process on :8080..."
  kill -9 $(lsof -ti :8080)
fi


# Adding message that displays backend local port
echo "Starting backend on http://127.0.0.1:8080 ..."
exec ./rml_backend
