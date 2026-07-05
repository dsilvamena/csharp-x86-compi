#!/usr/bin/env bash
# Regenera las dos variantes de la imagen de la gramatica:
#   grammar/grammar-light.svg  (tema claro de GitHub)
#   grammar/grammar-dark.svg   (tema oscuro de GitHub)
# El README las combina con <picture> segun prefers-color-scheme.
#
# Requiere: latex, dvisvgm (TeX Live / MacTeX) y python3.
#
#   bash grammar/build.sh
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cp "$DIR/grammar.tex" "$TMP/"
cd "$TMP"

# Inyecta un fondo solido cubriendo el viewBox. dvisvgm (via DVI) produce un SVG
# transparente; el rect da el color de fondo de cada tema y va justo tras <svg>.
inject_bg() {
  python3 - "$1" "$2" <<'PY'
import re, sys
path, fill = sys.argv[1], sys.argv[2]
svg = open(path, encoding="utf-8").read()
m = re.search(r"<svg\b[^>]*\bviewBox='([^']+)'[^>]*>", svg) \
    or re.search(r'<svg\b[^>]*\bviewBox="([^"]+)"[^>]*>', svg)
if not m:
    sys.exit("viewBox no encontrado en el SVG")
x, y, w, h = m.group(1).split()
rect = f"<rect x='{x}' y='{y}' width='{w}' height='{h}' fill='{fill}'/>"
svg = svg[:m.end()] + rect + svg[m.end():]
open(path, "w", encoding="utf-8").write(svg)
PY
}

# theme_flag  jobname  fondo
build_variant() {
  local flag="$1" job="$2" bg="$3"
  # LaTeX -> DVI (la ruta DVI evita depender de mutool/ghostscript en dvisvgm)
  latex -interaction=nonstopmode -halt-on-error -jobname="$job" \
    "${flag}\\input{grammar.tex}" >/dev/null
  # DVI -> SVG con las glifos como trazos (--no-fonts) para que GitHub lo
  # renderice identico sin depender de fuentes embebidas.
  dvisvgm --no-fonts "$job.dvi" -o "$job.svg" >/dev/null 2>&1
  inject_bg "$job.svg" "$bg"
  cp "$job.svg" "$DIR/$job.svg"
  echo "OK: $DIR/$job.svg"
}

build_variant ""              grammar-light "#ffffff"
build_variant "\\def\\darktheme{}" grammar-dark  "#0d1117"
