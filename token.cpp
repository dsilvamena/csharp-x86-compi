#include "token.h"

Token::Token(Type type, int line, int col)
    : type(type), text(""), line(line), col(col) {}

Token::Token(Type type, const std::string &text, int line, int col)
    : type(type), text(text), line(line), col(col) {}

std::string Token::typeName(Type t) {
  switch (t) {
    case CLASS: return "class";
    case STATIC: return "static";
    case STRUCT: return "struct";
    case NEW: return "new";
    case INT: return "int";
    case BOOL: return "bool";
    case CHAR: return "char";
    case STRING: return "string";
    case VOID: return "void";
    case IF: return "if";
    case ELSE: return "else";
    case WHILE: return "while";
    case FOR: return "for";
    case DO: return "do";
    case BREAK: return "break";
    case RETURN: return "return";
    case TRUE: return "true";
    case FALSE: return "false";
    case CONSOLE: return "Console";
    case NUM: return "NUM";
    case CHARLIT: return "CHARLIT";
    case STRINGLIT: return "STRINGLIT";
    case ID: return "ID";
    case PLUS: return "+";
    case MINUS: return "-";
    case MUL: return "*";
    case DIV: return "/";
    case MOD: return "%";
    case LT: return "<";
    case GT: return ">";
    case LE: return "<=";
    case GE: return ">=";
    case EQ: return "==";
    case NE: return "!=";
    case AND: return "&&";
    case OR: return "||";
    case NOT: return "!";
    case ASSIGN: return "=";
    case LPAREN: return "(";
    case RPAREN: return ")";
    case LBRACE: return "{";
    case RBRACE: return "}";
    case LBRACKET: return "[";
    case RBRACKET: return "]";
    case SEMICOL: return ";";
    case COMA: return ",";
    case DOT: return ".";
    case ERR: return "ERR";
    case END: return "END";
  }
  return "?";
}

std::ostream &operator<<(std::ostream &outs, const Token &tok) {
  outs << Token::typeName(tok.type);
  if (!tok.text.empty()) outs << "(" << tok.text << ")";
  return outs;
}
