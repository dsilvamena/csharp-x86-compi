#!/usr/bin/env bash
# =============================================================================
# gen_expected.sh — (Re)genera tests/expected/<name>.txt (correr en Linux).
# =============================================================================
# Compila y ejecuta cada input válido y guarda su salida como referencia. No
# inventa salidas: captura la ejecución real del binario nativo. Las referencias
# viven en tests/expected/ (versionadas) para sobrevivir a `make clean`.
# =============================================================================
set -eu
cd "$(dirname "$0")/.."
make >/dev/null
mkdir -p outputs tests/expected
for src in inputs/*.cs; do
  name=$(basename "$src" .cs)
  [[ "$name" == err_* ]] && continue
  ./compiler "$src" -o "outputs/$name.s" >/dev/null
  g++ -no-pie "outputs/$name.s" -o "outputs/$name"
  ./outputs/$name > "tests/expected/$name.txt"
  echo "expected <- $name"
done
