#!/usr/bin/env bash
# =============================================================================
# bench.sh — Lanza la suite de benchmarks en un contenedor Linux x86-64.
# Uso (desde la raiz del repo):
#   benchmarks/bench.sh
#
# Reconstruye tu compilador dentro del contenedor, instala clang, y compara
# tu compilador (--opt / --no-opt) contra gcc/clang sobre int32 con -fwrapv.
# Resultados en benchmarks/results/{results.csv,results.md}.
# =============================================================================
set -eu
HERE="$(cd "$(dirname "$0")/.." && pwd)"
IMG=gcc:13
DOCKER_BUILD_DIR=.docker-build

REBUILD="make BUILD_DIR=$DOCKER_BUILD_DIR clean >/dev/null 2>&1; make BUILD_DIR=$DOCKER_BUILD_DIR >/dev/null"
INSTALL_CLANG="apt-get update -qq >/dev/null 2>&1 && apt-get install -y -qq clang >/dev/null 2>&1 || echo 'aviso: no se pudo instalar clang, se omitira'"

docker run --rm --platform linux/amd64 -v "$HERE":/work -w /work "$IMG" \
  bash -lc "$REBUILD && { $INSTALL_CLANG; } && COMPILER=./$DOCKER_BUILD_DIR/compiler bash benchmarks/bench_inner.sh"
