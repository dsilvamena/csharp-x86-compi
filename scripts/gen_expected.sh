#!/usr/bin/env bash
# =============================================================================
# gen_expected.sh — (Re)genera tests/expected/<name>.txt (correr en Linux).
# =============================================================================
# Compila y ejecuta cada input válido y guarda su salida como referencia.
# =============================================================================
set -eu
cd "$(dirname "$0")/.."
make >/dev/null
mkdir -p outputs/asm outputs/bin tests/expected
COMPILER="${COMPILER:-./build/compiler}"
INPUT_DIR="${INPUT_DIR:-inputs}"
for src in "$INPUT_DIR"/*.cs; do
  name=$(basename "$src" .cs)
  [[ "$name" == err_* ]] && continue
  "$COMPILER" "$src" -o "outputs/asm/$name.s" >/dev/null
  g++ -no-pie "outputs/asm/$name.s" -o "outputs/bin/$name"
  ./outputs/bin/$name > "tests/expected/$name.txt"
  echo "expected <- $name"
done
