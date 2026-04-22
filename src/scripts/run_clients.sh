#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BIN="$SRC_DIR/cmake-build-release/TradingEcosystem"

if [ ! -f "$BIN" ]; then
    echo "Error: binary not found at $BIN"
    echo "Build the project first with: ./build.sh"
    exit 1
fi

echo "Using binary: $BIN"
echo "Starting clients..."

# Start dedicated exchange process first.
"$BIN" --exchange-only &
EXCHANGE_PID=$!
CLIENT_PIDS=()

cleanup() {
    # shellcheck disable=SC2317
    kill -TERM "$EXCHANGE_PID" 2>/dev/null || true
    # shellcheck disable=SC2317
    for _ in {1..20}; do
        if ! kill -0 "$EXCHANGE_PID" 2>/dev/null; then
            break
        fi
        sleep 0.1
    done

    # shellcheck disable=SC2317
    if kill -0 "$EXCHANGE_PID" 2>/dev/null; then
        kill -KILL "$EXCHANGE_PID" 2>/dev/null || true
    fi

    # shellcheck disable=SC2317
    wait "$EXCHANGE_PID" 2>/dev/null || true
}
trap cleanup EXIT

sleep 5

# ========================
# CLIENT 1 - MAKER (network)
# ========================
"$BIN" 1 MAKER \
    100 0.6 150 300 -100 \
    60 0.6 150 300 -100 &
CLIENT_PIDS+=($!)

sleep 5

# ========================
# CLIENT 2 - MAKER  (rede)
# ========================
"$BIN" 2 MAKER \
    2100 0.4 2150 2300 -1100 \
    260 0.8 2150 2300 -1100 &
CLIENT_PIDS+=($!)

sleep 2

# ========================
# CLIENT 3 - TAKER  (rede)
# ========================
"$BIN" 3 TAKER \
    300 0.8 350 300 -300 \
    60 0.7 350 300 -300 &
CLIENT_PIDS+=($!)

sleep 2

# ========================
# CLIENT 4 - TAKER  (rede)
# ========================
"$BIN" 4 TAKER \
    4100 0.8 4150 4300 -1100 \
    460 0.9 4150 4300 -1100 &
CLIENT_PIDS+=($!)

sleep 2

# ========================
# CLIENT 5 - RANDOM  (rede)
# ========================
"$BIN" 5 RANDOM &
CLIENT_PIDS+=($!)
FAIL=0
for pid in "${CLIENT_PIDS[@]}"; do
    if ! wait "$pid"; then
        FAIL=1
    fi
done

echo "All clients finished."
exit "$FAIL"
