#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <string>

class CompileError : public std::runtime_error {
public:
  std::string phase;
  std::string message;
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

#endif
