#!/usr/bin/env bash
# Regenera grammar/grammar.svg a partir de grammar/grammar.tex.
# Requiere: latex, dvisvgm (incluidos en una instalacion de TeX Live / MacTeX).
#
#   bash grammar/build.sh
#
# Si editas la gramatica, actualiza tambien el bloque EBNF de texto del README
# para que la imagen y el texto no se desincronicen.
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cp "$DIR/grammar.tex" "$TMP/"
cd "$TMP"

# 1) LaTeX -> DVI (la ruta DVI evita depender de mutool/ghostscript en dvisvgm)
latex -interaction=nonstopmode -halt-on-error grammar.tex >/dev/null

# 2) DVI -> SVG con las glifos convertidas a trazos (--no-fonts) para que
#    GitHub lo renderice de forma identica sin depender de fuentes embebidas.
dvisvgm --no-fonts grammar.dvi -o grammar.svg >/dev/null 2>&1

# 3) Inyecta un fondo blanco cubriendo el viewBox. dvisvgm produce un SVG
#    transparente; sin esto los no-terminales (gris oscuro) serian casi
#    invisibles en el modo oscuro de GitHub. El rect va justo tras <svg ...>.
python3 - grammar.svg <<'PY'
import re, sys
path = sys.argv[1]
svg = open(path, encoding="utf-8").read()
m = re.search(r"<svg\b[^>]*\bviewBox='([^']+)'[^>]*>", svg)
if not m:
    m = re.search(r'<svg\b[^>]*\bviewBox="([^"]+)"[^>]*>', svg)
if not m:
    sys.exit("viewBox no encontrado en el SVG")
x, y, w, h = m.group(1).split()
rect = f"<rect x='{x}' y='{y}' width='{w}' height='{h}' fill='#ffffff'/>"
svg = svg[:m.end()] + rect + svg[m.end():]
open(path, "w", encoding="utf-8").write(svg)
PY

cp grammar.svg "$DIR/grammar.svg"
echo "OK: $DIR/grammar.svg"
