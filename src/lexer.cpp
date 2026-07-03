#include "lexer.h"
#include "errors.h"
#include <cctype>
#include <unordered_map>

Lexer::Lexer(const std::string &source) : input(source) {}

char Lexer::current() const { return pos < input.size() ? input[pos] : '\0'; }

char Lexer::peek(int k) const {
  size_t p = pos + k;
  return p < input.size() ? input[p] : '\0';
}

bool Lexer::atEnd() const { return pos >= input.size(); }

void Lexer::advance() {
  if (atEnd()) return;
  if (input[pos] == '\n') {
    line++;
    col = 1;
  } else {
    col++;
  }
  pos++;
}

void Lexer::skipSpacesAndComments() {
  while (!atEnd()) {
    char c = current();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      advance();
    } else if (c == '/' && peek() == '/') {
      while (!atEnd() && current() != '\n') advance();
    } else if (c == '/' && peek() == '*') {
      int startLine = line, startCol = col;
      advance();
      advance();
      bool closed = false;
      while (!atEnd()) {
        if (current() == '*' && peek() == '/') {
          advance();
          advance();
          closed = true;
          break;
        }
        advance();
      }
      if (!closed)
        throw CompileError("lexico", "comentario de bloque sin cerrar", startLine, startCol);
    } else {
      break;
    }
  }
}

static const std::unordered_map<std::string, Token::Type> &keywords() {
  static const std::unordered_map<std::string, Token::Type> kw = {
      {"class", Token::CLASS},   {"static", Token::STATIC}, {"struct", Token::STRUCT},
      {"unsafe", Token::UNSAFE}, {"new", Token::NEW},       {"var", Token::VAR},
      {"int", Token::INT},
      {"bool", Token::BOOL},     {"char", Token::CHAR},     {"string", Token::STRING},
      {"void", Token::VOID},     {"if", Token::IF},         {"else", Token::ELSE},
      {"while", Token::WHILE},   {"for", Token::FOR},       {"do", Token::DO},
      {"break", Token::BREAK},   {"return", Token::RETURN}, {"true", Token::TRUE},
      {"false", Token::FALSE},   {"Console", Token::CONSOLE},
  };
  return kw;
}

Token Lexer::makeIdOrKeyword() {
  int startLine = line, startCol = col;
  std::string s;
  while (!atEnd() && (std::isalnum((unsigned char)current()) || current() == '_')) {
    s += current();
    advance();
  }
  auto it = keywords().find(s);
  if (it != keywords().end()) return Token(it->second, s, startLine, startCol);
  return Token(Token::ID, s, startLine, startCol);
}

Token Lexer::makeNumber() {
  int startLine = line, startCol = col;
  std::string s;
  while (!atEnd() && std::isdigit((unsigned char)current())) {
    s += current();
    advance();
  }
  return Token(Token::NUM, s, startLine, startCol);
}

static char readEscape(char e) {
  switch (e) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case '0': return '\0';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    default: return e;
  }
}

Token Lexer::makeChar() {
  int startLine = line, startCol = col;
  advance();
  if (atEnd()) throw CompileError("lexico", "literal de caracter sin cerrar", startLine, startCol);
  char value;
  if (current() == '\\') {
    advance();
    if (atEnd()) throw CompileError("lexico", "escape de caracter incompleto", startLine, startCol);
    value = readEscape(current());
    advance();
  } else {
    value = current();
    advance();
  }
  if (current() != '\'')
    throw CompileError("lexico", "literal de caracter mal formado (se esperaba ')", line, col);
  advance();
  return Token(Token::CHARLIT, std::to_string((int)(unsigned char)value), startLine, startCol);
}

Token Lexer::makeString() {
  int startLine = line, startCol = col;
  advance();
  std::string s;
  while (!atEnd() && current() != '"') {
    if (current() == '\n')
      throw CompileError("lexico", "string sin cerrar (salto de linea)", startLine, startCol);
    if (current() == '\\') {
      advance();
      if (atEnd()) break;
      s += readEscape(current());
      advance();
    } else {
      s += current();
      advance();
    }
  }
  if (atEnd()) throw CompileError("lexico", "string sin cerrar", startLine, startCol);
  advance();
  return Token(Token::STRINGLIT, s, startLine, startCol);
}

Token Lexer::nextToken() {
  skipSpacesAndComments();
  if (atEnd()) return Token(Token::END, line, col);

  char c = current();
  int L = line, C = col;

  if (std::isalpha((unsigned char)c) || c == '_') return makeIdOrKeyword();
  if (std::isdigit((unsigned char)c)) return makeNumber();
  if (c == '\'') return makeChar();
  if (c == '"') return makeString();

  switch (c) {
    case '=':
      if (peek() == '=') { advance(); advance(); return Token(Token::EQ, "==", L, C); }
      advance(); return Token(Token::ASSIGN, "=", L, C);
    case '!':
      if (peek() == '=') { advance(); advance(); return Token(Token::NE, "!=", L, C); }
      advance(); return Token(Token::NOT, "!", L, C);
    case '<':
      if (peek() == '=') { advance(); advance(); return Token(Token::LE, "<=", L, C); }
      advance(); return Token(Token::LT, "<", L, C);
    case '>':
      if (peek() == '=') { advance(); advance(); return Token(Token::GE, ">=", L, C); }
      advance(); return Token(Token::GT, ">", L, C);
    case '&':
      if (peek() == '&') { advance(); advance(); return Token(Token::AND, "&&", L, C); }
      advance(); return Token(Token::AMP, "&", L, C);
    case '|':
      if (peek() == '|') { advance(); advance(); return Token(Token::OR, "||", L, C); }
      break;
    case '+': advance(); return Token(Token::PLUS, "+", L, C);
    case '-': advance(); return Token(Token::MINUS, "-", L, C);
    case '*': advance(); return Token(Token::MUL, "*", L, C);
    case '/': advance(); return Token(Token::DIV, "/", L, C);
    case '%': advance(); return Token(Token::MOD, "%", L, C);
    case '(': advance(); return Token(Token::LPAREN, "(", L, C);
    case ')': advance(); return Token(Token::RPAREN, ")", L, C);
    case '{': advance(); return Token(Token::LBRACE, "{", L, C);
    case '}': advance(); return Token(Token::RBRACE, "}", L, C);
    case '[': advance(); return Token(Token::LBRACKET, "[", L, C);
    case ']': advance(); return Token(Token::RBRACKET, "]", L, C);
    case ';': advance(); return Token(Token::SEMICOL, ";", L, C);
    case ',': advance(); return Token(Token::COMA, ",", L, C);
    case '.': advance(); return Token(Token::DOT, ".", L, C);
  }

  throw CompileError("lexico", std::string("caracter no reconocido: '") + c + "'", L, C);
}
