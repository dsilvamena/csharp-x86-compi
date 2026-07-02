#!/usr/bin/env bash
# =============================================================================
# docker.sh — Ejecuta el compilador en un contenedor Linux x86-64 (para macOS
# / Apple Silicon, donde el .s AT&T Linux no corre nativo).
# =============================================================================
# Uso:
#   scripts/docker.sh test            # make + suite completa (run_tests.sh)
#   scripts/docker.sh gen             # regenera tests/expected/
#   scripts/docker.sh run inputs/x.cs # compila, linka y ejecuta un programa
#   scripts/docker.sh shell           # shell interactivo dentro del contenedor
# =============================================================================
set -eu
HERE="$(cd "$(dirname "$0")/.." && pwd)"
IMG=gcc:13
DOCK=(docker run --rm --platform linux/amd64 -v "$HERE":/work -w /work "$IMG")

# Recompila siempre desde cero: el binario del host (macOS/arm64) no corre en
# el contenedor Linux, así que hay que reconstruir dentro.
REBUILD="make clean >/dev/null 2>&1; make >/dev/null"

cmd="${1:-test}"
case "$cmd" in
  test) "${DOCK[@]}" bash -lc "$REBUILD && bash scripts/run_tests.sh" ;;
  gen)  "${DOCK[@]}" bash -lc "$REBUILD && bash scripts/gen_expected.sh" ;;
  run)
    src="${2:?uso: scripts/docker.sh run inputs/archivo.cs}"
    name="$(basename "$src" .cs)"
    "${DOCK[@]}" bash -lc "$REBUILD && ./compiler '$src' -o outputs/$name.s && g++ -no-pie outputs/$name.s -o outputs/$name && echo '----- salida -----' && ./outputs/$name"
    ;;
  shell) docker run --rm -it --platform linux/amd64 -v "$HERE":/work -w /work "$IMG" bash ;;
  *) echo "comando desconocido: $cmd (test|gen|run|shell)"; exit 2 ;;
esac
