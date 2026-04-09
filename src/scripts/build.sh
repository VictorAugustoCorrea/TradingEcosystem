#!/bin/bash

set -e

# ----------------------------
# Detecta binários
# ----------------------------
CMAKE=$(which cmake)
NINJA=$(which ninja)

if [ -z "$CMAKE" ]; then
    echo "Erro: cmake não encontrado."
    echo "Instale com: sudo apt install cmake ninja-build"
    exit 1
fi

if [ -z "$NINJA" ]; then
    echo "Erro: ninja não encontrado."
    echo "Instale com: sudo apt install ninja-build"
    exit 1
fi

# ----------------------------
# Caminhos
# scripts/ → src/ → TradingEcosystem/
#
# SCRIPT_DIR  = .../TradingEcosystem/src/scripts/
# SRC_DIR     = .../TradingEcosystem/src/          (onde ficam os cmake-build-*)
# PROJECT_ROOT= .../TradingEcosystem/              (onde fica o CMakeLists.txt)
# ----------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Project root: $PROJECT_ROOT"
echo "Src dir:      $SRC_DIR"
echo "Using cmake:  $CMAKE"
echo "Using ninja:  $NINJA"

# ----------------------------
# Função de build
# ----------------------------
build() {
    local BUILD_TYPE="$1"
    local BUILD_DIR="$SRC_DIR/cmake-build-${BUILD_TYPE,,}"

    echo "========================"
    echo "BUILD $BUILD_TYPE"
    echo "========================"

    mkdir -p "$BUILD_DIR"

    "$CMAKE" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_MAKE_PROGRAM="$NINJA" \
        -G Ninja \
        -S "$PROJECT_ROOT" \
        -B "$BUILD_DIR"

    "$CMAKE" --build "$BUILD_DIR" --target clean
    # shellcheck disable=SC2046
    "$CMAKE" --build "$BUILD_DIR" --target all -j$(nproc)
}

# ----------------------------
# Executa builds
# ----------------------------
build Release
build Debug

echo "========================"
echo "BUILD FINALIZADO"
echo "========================"
date