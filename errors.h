#ifndef ERRORS_H
#define ERRORS_H

// =============================================================================
// errors.h — Excepción uniforme de compilación (fase + línea + columna).
// =============================================================================
// Centraliza errores léxicos, sintácticos y semánticos con fase, línea y
// columna. Evita duplicar el formato en lexer/parser/typechecker y permite que
// main.cpp maneje todos los fallos desde un único punto.
// =============================================================================

#include <stdexcept>
#include <string>

class CompileError : public std::runtime_error {
public:
  std::string phase;    // "lexico" | "sintactico" | "semantico"
  std::string message;  // mensaje crudo, sin el prefijo de formato (para --json)
  int line;
  int col;

  CompileError(const std::string &phase, const std::string &message, int line, int col)
      : std::runtime_error(format(phase, message, line, col)),
        phase(phase), message(message), line(line), col(col) {}

private:
  static std::string format(const std::string &phase, const std::string &message,
                            int line, int col) {
    return "[error " + phase + "] linea " + std::to_string(line) + ", columna " +
           std::to_string(col) + ": " + message;
  }
};

#endif // ERRORS_H
