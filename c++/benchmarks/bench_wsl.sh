#!/usr/bin/env bash
# =============================================================================
# bench_wsl.sh — Corre la suite directamente en WSL/Linux x86-64, sin Docker.
#
# Requiere que el sistema tenga: make, g++, gcc, clang (opcional) y dotnet
# (opcional para dotnet-aot). Si clang o dotnet faltan, bench_inner.sh omite esas
# configuraciones y deja una nota al final.
# =============================================================================
set -eu

cd "$(dirname "$0")/.."

if [ "$(uname -s)" != "Linux" ] || [ "$(uname -m)" != "x86_64" ]; then
  echo "bench_wsl.sh requiere WSL/Linux x86-64. En macOS usa benchmarks/bench.sh." >&2
  exit 1
fi

missing=0
for cmd in make g++ gcc; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "falta dependencia requerida: $cmd" >&2
    missing=1
  fi
done

if [ "$missing" -ne 0 ]; then
  echo "Instala las dependencias base, por ejemplo:" >&2
  echo "  sudo apt update && sudo apt install -y build-essential" >&2
  exit 1
fi

BUILD_DIR="${BUILD_DIR:-.wsl-build}"
COMPILER="${COMPILER:-./$BUILD_DIR/compiler}"

make BUILD_DIR="$BUILD_DIR" clean >/dev/null 2>&1
make BUILD_DIR="$BUILD_DIR" >/dev/null

COMPILER="$COMPILER" bash benchmarks/bench_inner.sh
