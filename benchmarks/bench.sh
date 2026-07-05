#!/usr/bin/env bash
# =============================================================================
# bench.sh — Lanza la suite de benchmarks en un contenedor Linux x86-64.
# Uso (desde la raiz del repo):
#   benchmarks/bench.sh
#
# Reconstruye tu compilador dentro del contenedor, instala herramientas nativas,
# y compara tu compilador (--opt / --no-opt) contra gcc, clang y .NET Native AOT.
# Resultados en benchmarks/results/{results.csv,results.md}.
# =============================================================================
set -eu
HERE="$(cd "$(dirname "$0")/.." && pwd)"
IMG=mcr.microsoft.com/dotnet/sdk:8.0
DOCKER_BUILD_DIR=.docker-build

REBUILD="make BUILD_DIR=$DOCKER_BUILD_DIR clean >/dev/null 2>&1; make BUILD_DIR=$DOCKER_BUILD_DIR >/dev/null"
INSTALL_TOOLS="apt-get update -qq >/dev/null 2>&1 && apt-get install -y -qq build-essential clang zlib1g-dev >/dev/null 2>&1 || echo 'aviso: no se pudieron instalar herramientas nativas completas'"

docker run --rm --platform linux/amd64 -e BENCHES="${BENCHES:-}" -v "$HERE":/work -w /work "$IMG" \
  bash -lc "{ $INSTALL_TOOLS; } && $REBUILD && COMPILER=./$DOCKER_BUILD_DIR/compiler bash benchmarks/bench_inner.sh"
