#!/usr/bin/env bash
# =============================================================================
# run_tests.sh — Suite de pruebas (correr en Linux x86-64 con g++).
# =============================================================================
# Para cada inputs/*.cs:
#   · err_*     -> deben FALLAR en compilación con la fase esperada
#                      (léxico/sintáctico/semántico, tomada del nombre del archivo).
#   · resto         -> compilar a .s con --opt y --no-opt, ensamblar con
#                      `g++ -no-pie`, ejecutar y comparar la salida con
#                      tests/expected/<name>.txt. Además se exige que --opt y
#                      --no-opt produzcan la MISMA salida.
#
# Las salidas de referencia viven en tests/expected/.
# =============================================================================
set -u
cd "$(dirname "$0")/.."

COMPILER="${COMPILER:-./build/compiler}"
INPUT_DIR="${INPUT_DIR:-inputs}"

if [ ! -x "$COMPILER" ]; then
  echo "compilando..."; make >/dev/null || { echo "build fallo"; exit 1; }
fi
mkdir -p outputs/asm outputs/bin tests/expected

PASS=0; FAIL=0

run_one() {  # <name> <src> <mode>
  local name="$1" src="$2" mode="$3"
  "$COMPILER" "$src" "$mode" -o "outputs/asm/$name.s" >/dev/null 2>/tmp/cerr || { echo "FAIL $name [$mode] (compile)"; cat /tmp/cerr; return 1; }
  g++ -no-pie "outputs/asm/$name.s" -o "outputs/bin/$name" 2>/tmp/aerr || { echo "FAIL $name [$mode] (assemble)"; cat /tmp/aerr; return 1; }
  ./outputs/bin/$name
}

for src in "$INPUT_DIR"/*.cs; do
  name=$(basename "$src" .cs)
  if [[ "$name" == err_* ]]; then
    # La fase esperada es el sufijo del nombre: err_01_lexico -> lexico.
    phase="${name##*_}"
    if "$COMPILER" "$src" >/dev/null 2>/tmp/cerr; then
      echo "FAIL $name (deberia fallar la compilacion)"; FAIL=$((FAIL+1))
    elif ! grep -q "error $phase" /tmp/cerr; then
      echo "FAIL $name (fase != $phase)  ->  $(head -1 /tmp/cerr)"; FAIL=$((FAIL+1))
    else
      echo "ok   $name  ->  $(head -1 /tmp/cerr)"; PASS=$((PASS+1))
    fi
    continue
  fi

  got_noopt=$(run_one "$name" "$src" "--no-opt") || { FAIL=$((FAIL+1)); continue; }
  got_opt=$(run_one "$name" "$src" "--opt") || { FAIL=$((FAIL+1)); continue; }

  if [[ "$got_noopt" != "$got_opt" ]]; then
    echo "FAIL $name (--opt y --no-opt difieren!)"; FAIL=$((FAIL+1)); continue
  fi

  exp="tests/expected/$name.txt"
  if [[ -f "$exp" ]]; then
    if [[ "$got_opt" == "$(cat "$exp")" ]]; then
      echo "ok   $name"; PASS=$((PASS+1))
    else
      echo "FAIL $name (salida != expected)"
      echo "  esperado: $(tr '\n' '|' < "$exp")"
      echo "  obtenido: $(echo "$got_opt" | tr '\n' '|')"
      FAIL=$((FAIL+1))
    fi
  else
    echo "??   $name (sin expected; salida): $(echo "$got_opt" | tr '\n' '|')"
  fi
done

echo "------------------------------------------"
echo "PASS=$PASS  FAIL=$FAIL"
[[ $FAIL -eq 0 ]]
