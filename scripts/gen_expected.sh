#!/usr/bin/env bash
# =============================================================================
# gen_expected.sh — (Re)genera tests/expected/<name>.txt (correr en Linux).
# =============================================================================
# Compila y ejecuta cada input válido y guarda su salida como referencia.
# =============================================================================
set -eu
cd "$(dirname "$0")/.."
make >/dev/null
mkdir -p outputs tests/expected
COMPILER="${COMPILER:-./build/compiler}"
INPUT_DIR="${INPUT_DIR:-inputs}"
for src in "$INPUT_DIR"/*.cs; do
  name=$(basename "$src" .cs)
  [[ "$name" == err_* ]] && continue
  "$COMPILER" "$src" -o "outputs/$name.s" >/dev/null
  g++ -no-pie "outputs/$name.s" -o "outputs/$name"
  ./outputs/$name > "tests/expected/$name.txt"
  echo "expected <- $name"
done
