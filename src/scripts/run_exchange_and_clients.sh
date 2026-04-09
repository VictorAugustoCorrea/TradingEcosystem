#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_SCRIPT="$SCRIPT_DIR/build.sh"
RUN_CLIENTS_SCRIPT="$SCRIPT_DIR/run_clients.sh"

if [ ! -f "$BUILD_SCRIPT" ]; then
    echo "Error: build.sh not found at $BUILD_SCRIPT"
    exit 1
fi

if [ ! -f "$RUN_CLIENTS_SCRIPT" ]; then
    echo "Error: run_clients.sh not found at $RUN_CLIENTS_SCRIPT"
    exit 1
fi

# ========================
# BUILD
# ========================
bash "$BUILD_SCRIPT"

date

echo "---------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "Starting Exchange + Clients..."
echo "---------------------------------------------------------------------------------------------------------------------------------------------------------"

bash "$RUN_CLIENTS_SCRIPT"

date

echo "---------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "All done."
echo "---------------------------------------------------------------------------------------------------------------------------------------------------------"