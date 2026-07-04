#!/usr/bin/env bash
# =============================================================================
# bench_inner.sh — Corre DENTRO del contenedor Linux x86-64 (gcc:13).
# No lo invoques directo en macOS; usa: benchmarks/bench.sh
#
# Para cada benchmark mide, en cada configuracion de compilador:
#   - tiempo de compilacion (fuente -> ejecutable), mejor de 3
#   - tiempo de ejecucion del binario, mejor de 5
#   - lineas de ensamblador generado
#   - tamano del ejecutable
#   - salida del programa (para verificar que TODAS coincidan)
#
# Configuraciones:
#   mine-noopt : tu compilador --no-opt  + g++ -no-pie (link)
#   mine-opt   : tu compilador --opt     + g++ -no-pie (link)
#   gcc-O0     : gcc -O0 -fwrapv
#   gcc-O2     : gcc -O2 -fwrapv
#   clang-O2   : clang -O2 -fwrapv   (si clang esta disponible)
# =============================================================================
set -eu

COMPILER="${COMPILER:-./.docker-build/compiler}"
BENCHES="${BENCHES:-fib hashloop sieve matmul}"
CS_DIR=benchmarks/cs
C_DIR=benchmarks/c
OUT=benchmarks/results
WORK="$OUT/work"
mkdir -p "$WORK"

CSV="$OUT/results.csv"
MD="$OUT/results.md"

HAVE_CLANG=0
if command -v clang >/dev/null 2>&1; then HAVE_CLANG=1; fi

# --- helper: mide el mejor tiempo (ns) de correr un comando N veces ----------
timeit() {
  local reps=$1; shift
  local best="" s e d
  for ((r = 0; r < reps; r++)); do
    s=$(date +%s%N)
    "$@" >/dev/null 2>&1
    e=$(date +%s%N)
    d=$((e - s))
    if [ -z "$best" ] || [ "$d" -lt "$best" ]; then best=$d; fi
  done
  echo "$best"
}

ns_to_ms() { awk -v n="$1" 'BEGIN { printf "%.2f", n/1000000.0 }'; }

# --- comandos de build (fuente -> ejecutable) --------------------------------
build_mine() { # <bench> <optflag> <tag>
  "$COMPILER" "$CS_DIR/$1.cs" "$2" -o "$WORK/$1.$3.s" >/dev/null 2>&1
  g++ -no-pie "$WORK/$1.$3.s" -o "$WORK/$1.$3" >/dev/null 2>&1
}
build_gcc() { # <bench> <level>
  gcc -O"$2" -fwrapv "$C_DIR/$1.c" -o "$WORK/$1.gcc$2" >/dev/null 2>&1
}
build_clang() { # <bench>
  clang -O2 -fwrapv "$C_DIR/$1.c" -o "$WORK/$1.clang2" >/dev/null 2>&1
}

echo "benchmark,config,output,compile_ms,run_ms,asm_lines,bin_bytes" > "$CSV"

# --- una fila: compila (medido), captura asm/tamano/salida, corre (medido) ---
run_config() { # <bench> <config> <exe_path> <asm_path> <build_fn...>
  local bench="$1" config="$2" exe="$3" asm="$4"; shift 4
  local cns rns cms rms lines bytes output

  cns=$(timeit 3 "$@")             # tiempo de compilacion
  "$@"                             # deja artefactos finales en disco
  cms=$(ns_to_ms "$cns")

  if [ -f "$asm" ]; then lines=$(wc -l < "$asm" | tr -d ' '); else lines=NA; fi
  bytes=$(stat -c%s "$exe" 2>/dev/null || echo NA)
  output=$("$exe" 2>/dev/null | tr '\n' '|' | sed 's/|$//')

  rns=$(timeit 5 "$exe")           # tiempo de ejecucion
  rms=$(ns_to_ms "$rns")

  echo "$bench,$config,$output,$cms,$rms,$lines,$bytes" >> "$CSV"
  printf "  %-10s salida=%-12s compile=%8sms  run=%9sms  asm=%6s  bin=%s\n" \
    "$config" "$output" "$cms" "$rms" "$lines" "$bytes"
}

for b in $BENCHES; do
  echo "== $b =="

  # Ensamblador de referencia de gcc/clang (para contar lineas; no cronometrado)
  gcc   -O0 -fwrapv -S "$C_DIR/$b.c" -o "$WORK/$b.gcc0.s"   2>/dev/null || true
  gcc   -O2 -fwrapv -S "$C_DIR/$b.c" -o "$WORK/$b.gcc2.s"   2>/dev/null || true
  [ "$HAVE_CLANG" = 1 ] && clang -O2 -fwrapv -S "$C_DIR/$b.c" -o "$WORK/$b.clang2.s" 2>/dev/null || true

  run_config "$b" mine-noopt "$WORK/$b.noopt" "$WORK/$b.noopt.s" build_mine "$b" --no-opt noopt
  run_config "$b" mine-opt   "$WORK/$b.opt"   "$WORK/$b.opt.s"   build_mine "$b" --opt   opt
  run_config "$b" gcc-O0     "$WORK/$b.gcc0"  "$WORK/$b.gcc0.s"  build_gcc  "$b" 0
  run_config "$b" gcc-O2     "$WORK/$b.gcc2"  "$WORK/$b.gcc2.s"  build_gcc  "$b" 2
  if [ "$HAVE_CLANG" = 1 ]; then
    run_config "$b" clang-O2 "$WORK/$b.clang2" "$WORK/$b.clang2.s" build_clang "$b"
  fi
done

# --- genera tabla Markdown a partir del CSV ----------------------------------
awk -F, '
  NR==1 { next }
  {
    b=$1; cfg=$2; out=$3; cms=$4; rms=$5; asm=$6; bin=$7
    if (!(b in seen)) { order[++n]=b; seen[b]=1 }
    key=b SUBSEP cfg
    O[key]=out; C[key]=cms; R[key]=rms; A[key]=asm; B[key]=bin
    if (!(b in ref)) { ref[b]=out }               # salida de la 1a config
    if (out != ref[b]) { mismatch[b]=1 }
    cfgs[cfg]=1
  }
  END {
    ncfg=split("mine-noopt mine-opt gcc-O0 gcc-O2 clang-O2", clist, " ")
    print "# Resultados de benchmarks\n"
    print "Tiempos en milisegundos (mejor de 3 compilaciones / 5 ejecuciones).\n"
    for (i=1;i<=n;i++) {
      b=order[i]
      status = (b in mismatch) ? "**SALIDAS NO COINCIDEN**" : "salidas OK (todas iguales)"
      print "## " b "  (" status ")\n"
      print "| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |"
      print "|---|---|---|---|---|---|"
      for (j=1;j<=ncfg;j++) {
        cfg=clist[j]; key=b SUBSEP cfg
        if (!(key in O)) continue
        printf "| %s | %s | %s | %s | %s | %s |\n", cfg, O[key], C[key], R[key], A[key], B[key]
      }
      print ""
    }
  }
' "$CSV" > "$MD"

echo
echo "CSV : $CSV"
echo "MD  : $MD"
[ "$HAVE_CLANG" = 1 ] || echo "NOTA: clang no estaba disponible; se omitio clang-O2."
