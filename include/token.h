#ifndef TOKEN_H
#define TOKEN_H

#include <ostream>
#include <string>

class Token {
public:
  enum Type {
    CLASS, STATIC, STRUCT, UNSAFE, NEW,
    INT, BOOL, CHAR, STRING, VOID,
    IF, ELSE, WHILE, FOR, DO, BREAK, RETURN,
    TRUE, FALSE,
    CONSOLE,
    NUM, CHARLIT, STRINGLIT, ID,
    PLUS, MINUS, MUL, DIV, MOD,
    LT, GT, LE, GE, EQ, NE,
    AND, OR, NOT, AMP,
    ASSIGN,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOL, COMA, DOT,
    ERR, END
  };

  Type type;
  std::string text;
  int line;
  int col;

  Token(Type type, int line = 0, int col = 0);
  Token(Type type, const std::string &text, int line, int col);

  static std::string typeName(Type t);

  friend std::ostream &operator<<(std::ostream &outs, const Token &tok);
};

#endif
