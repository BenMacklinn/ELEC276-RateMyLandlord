#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

# Build backend first (fast if no changes)
bash "$ROOT/run_backend.sh" &            # starts backend and keeps it in foreground for that subshell
BACK_PID=$!

# Start frontend
bash "$ROOT/run_frontend.sh" &           # starts vite
FRONT_PID=$!
wait
