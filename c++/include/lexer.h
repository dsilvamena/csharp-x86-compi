#ifndef LEXER_H
#define LEXER_H

#include <string>
#include "token.h"

class Lexer {
private:
  std::string input;
  size_t pos = 0;
  int line = 1;
  int col = 1;

  char current() const;
  char peek(int k = 1) const;
  void advance();
  bool atEnd() const;
  void skipSpacesAndComments();

  Token makeIdOrKeyword();
  Token makeNumber();
  Token makeChar();
  Token makeString();

public:
  explicit Lexer(const std::string &source);
  Token nextToken();
};

#endif
