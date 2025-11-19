#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/frontend"

command -v npm >/dev/null 2>&1 || { echo "npm not found. Install Node.js (>=18)."; exit 1; }

# Free the port (macOS/Linux)
if lsof -ti :5173 >/dev/null 2>&1; then
  echo "Killing process on :5173..."
  kill -9 $(lsof -ti :5173)
fi

npm install
echo "Starting frontend on http://127.0.0.1:5173 ..."
exec npm run dev
