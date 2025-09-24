#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/frontend"

command -v npm >/dev/null 2>&1 || { echo "npm not found. Install Node.js (>=18)."; exit 1; }

npm install
echo "Starting frontend on http://127.0.0.1:5173 ..."
exec npm run dev
