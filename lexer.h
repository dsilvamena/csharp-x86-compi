#ifndef LEXER_H
#define LEXER_H

// =============================================================================
// lexer.h — Analizador léxico.
// =============================================================================
// Reconoce el subconjunto de C#: keywords, identificadores, enteros, char y
// string con escapes básicos, operadores, delimitadores y comentarios // y /* */.
// =============================================================================

#include <string>
#include "token.h"

class Lexer {
private:
  std::string input;
  size_t pos = 0;
  int line = 1;
  int col = 1;

  char current() const;          // carácter actual ('\0' si fin)
  char peek(int k = 1) const;    // carácter k posiciones adelante
  void advance();                // avanza un carácter actualizando línea/col
  bool atEnd() const;
  void skipSpacesAndComments();  // espacios + // y /* ... */

  Token makeIdOrKeyword();
  Token makeNumber();
  Token makeChar();
  Token makeString();

public:
  explicit Lexer(const std::string &source);
  Token nextToken();   // siguiente token; END al terminar
};

#endif // LEXER_H
