#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include <deque>
#include <set>

class Parser {
private:
  Lexer &lexer;
  Token current;
  Token previous;
  std::deque<Token> lookahead;

  bool match(Token::Type t);
  bool check(Token::Type t) const;
  void advance();
  bool isAtEnd() const;
  const Token &peek(size_t n = 1);
  Token expect(Token::Type t, const std::string &what);
  [[noreturn]] void error(const std::string &what);

  bool isTypeStart() const;
  bool looksLikeVarDecl();
  std::string parseType();
  std::set<std::string> structNames;

  StructDecl *parseStructDecl();
  ClassDecl *parseClassDecl();
  MethodDecl *parseMethodDecl();
  Block *parseBlock();
  Stm *parseStatement();
  Stm *parseVarDecNoSemi();
  Stm *parseSimpleStmNoSemi();
  Stm *parseIf();
  Stm *parseWhile();
  Stm *parseFor();
  Stm *parseDoWhile();
  Stm *parseReturn();
  Stm *parseBreak();
  Stm *parsePrint();

  Exp *parseExpr();
  Exp *parseLogicalOr();
  Exp *parseLogicalAnd();
  Exp *parseEquality();
  Exp *parseRelational();
  Exp *parseAdditive();
  Exp *parseMultiplicative();
  Exp *parseUnary();
  Exp *parsePrimary();
  std::vector<Exp *> parseArgs();

public:
  explicit Parser(Lexer &lexer);
  Program *parseProgram();
};

#endif
