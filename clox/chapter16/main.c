#include <stdio.h>
#include "scanner.h"

static const char* tokenName(TokenType type) {
  static const char* names[] = {
    "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE", "COMMA",
    "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR", "BANG",
    "BANG_EQUAL", "EQUAL", "EQUAL_EQUAL", "GREATER", "GREATER_EQUAL",
    "LESS", "LESS_EQUAL", "IDENTIFIER", "STRING", "NUMBER", "AND",
    "CLASS", "ELSE", "FALSE", "FOR", "FUN", "IF", "NIL", "OR",
    "PRINT", "RETURN", "SUPER", "THIS", "TRUE", "VAR", "WHILE", "ERROR", "EOF"
  };
  return names[type];
}
int main(int argc, char** argv) {
  if (argc != 2) { fprintf(stderr, "Usage: %s 'source code'\n", argv[0]); return 64; }
  initScanner(argv[1]);
  for (;;) {
    Token token = scanToken();
    printf("line %d %-16s '%.*s'\n", token.line, tokenName(token.type),
           token.length, token.start);
    if (token.type == TOKEN_EOF || token.type == TOKEN_ERROR) break;
  }
  return 0;
}
