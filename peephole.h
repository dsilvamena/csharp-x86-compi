#ifndef PEEPHOLE_H
#define PEEPHOLE_H

// =============================================================================
// peephole.h — Optimización peephole sobre el ensamblador textual.
// =============================================================================
// Pasa final, segura, sobre el .s ya generado:
//   1. elimina código inalcanzable tras 'jmp'/'ret' hasta la siguiente etiqueta;
//   2. elimina 'movq %rX, %rX' redundantes;
//   3. elimina 'jmp L' cuando la línea siguiente es justamente 'L:'.
// Se mantiene como paso textual independiente del codegen para hacerlo
// =============================================================================

#include <string>

// Devuelve el .s optimizado; si removed != nullptr, escribe cuántas líneas se eliminaron.
std::string peephole(const std::string &asmText, int *removed = nullptr);

#endif // PEEPHOLE_H
