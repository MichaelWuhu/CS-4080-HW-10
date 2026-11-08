#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "object.h"
#include "scanner.h"

typedef struct { Token name; int depth; bool isConst; } Local;
typedef struct Loop {
  int continueTarget;
  int scopeDepth;
  struct Loop* enclosing;
} Loop;
typedef struct { Token current; Token previous; bool hadError; bool panicMode; } Parser;
typedef enum {
  PREC_NONE, PREC_ASSIGNMENT, PREC_OR, PREC_AND, PREC_EQUALITY,
  PREC_COMPARISON, PREC_TERM, PREC_FACTOR, PREC_UNARY, PREC_CALL, PREC_PRIMARY
} Precedence;
typedef void (*ParseFn)(bool canAssign);
typedef struct { ParseFn prefix; ParseFn infix; Precedence precedence; } ParseRule;

static ParseRule* getRule(TokenType type);
static Parser parser;
static Chunk* compilingChunk;
static Local* locals;
static int localCount;
static int localCapacity;
static int scopeDepth;
static Loop* currentLoop;

static Chunk* currentChunk(void) { return compilingChunk; }
static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF) fprintf(stderr, " at end");
  else if (token->type != TOKEN_ERROR) fprintf(stderr, " at '%.*s'", token->length, token->start);
  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}
static void error(const char* message) { errorAt(&parser.previous, message); }
static void errorAtCurrent(const char* message) { errorAt(&parser.current, message); }
static void advance(void) {
  parser.previous = parser.current;
  for (;;) { parser.current = scanToken(); if (parser.current.type != TOKEN_ERROR) break; errorAtCurrent(parser.current.start); }
}
static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) { advance(); return; }
  errorAtCurrent(message);
}
static bool match(TokenType type) { if (parser.current.type != type) return false; advance(); return true; }
static void emitByte(uint8_t byte) { writeChunk(currentChunk(), byte, parser.previous.line); }
static void emitBytes(uint8_t a, uint8_t b) { emitByte(a); emitByte(b); }
static void emitShort(uint8_t opcode, uint16_t operand) {
  emitByte(opcode); emitByte((uint8_t)(operand & 0xff)); emitByte((uint8_t)(operand >> 8));
}
static void emitReturn(void) { emitBytes(OP_NIL, OP_RETURN); }
static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) { error("Too many constants in one chunk."); return 0; }
  return (uint8_t)constant;
}
static void emitConstant(Value value) { emitBytes(OP_CONSTANT, makeConstant(value)); }
static uint8_t identifierConstant(const Token* name) { return makeConstant(OBJ_VAL(copyString(name->start, name->length))); }
static bool identifiersEqual(const Token* a, const Token* b) {
  return a->length == b->length && memcmp(a->start, b->start, (size_t)a->length) == 0;
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction); emitByte(0xff); emitByte(0xff); return currentChunk()->count - 3;
}
static void patchJump(int offset) {
  int jump = currentChunk()->count - offset - 3;
  if (jump > UINT16_MAX) error("Too much code to jump over.");
  currentChunk()->code[offset + 1] = (uint8_t)(jump >> 8);
  currentChunk()->code[offset + 2] = (uint8_t)jump;
}
static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);
  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");
  emitByte((uint8_t)(offset >> 8)); emitByte((uint8_t)offset);
}

static void declareVariable(bool isConst) {
  if (scopeDepth == 0) return;
  Token* name = &parser.previous;
  for (int i = localCount - 1; i >= 0; i--) {
    Local* local = &locals[i];
    if (local->depth != -1 && local->depth < scopeDepth) break;
    if (identifiersEqual(name, &local->name)) error("Already a variable with this name in this scope.");
  }
  if (localCount == localCapacity) {
    localCapacity = localCapacity < 8 ? 8 : localCapacity * 2;
    locals = (Local*)realloc(locals, sizeof(Local) * (size_t)localCapacity);
    if (!locals) { fprintf(stderr, "Out of memory while creating locals.\n"); exit(1); }
  }
  locals[localCount++] = (Local){*name, -1, isConst};
}
static void markInitialized(void) { if (scopeDepth > 0) locals[localCount - 1].depth = scopeDepth; }
static int resolveLocal(const Token* name) {
  for (int i = localCount - 1; i >= 0; i--) {
    if (identifiersEqual(name, &locals[i].name)) {
      if (locals[i].depth == -1) error("Can't read local variable in its own initializer.");
      return i;
    }
  }
  return -1;
}
static void defineVariable(uint8_t global, bool isConst) {
  if (scopeDepth > 0) { markInitialized(); return; }
  emitBytes(isConst ? OP_DEFINE_CONST_GLOBAL : OP_DEFINE_GLOBAL, global);
}
static void beginScope(void) { scopeDepth++; }
static void endScope(void) {
  scopeDepth--;
  while (localCount > 0 && locals[localCount - 1].depth > scopeDepth) { emitByte(OP_POP); localCount--; }
}
static void emitLoopCleanup(int targetDepth) {
  for (int i = localCount - 1; i >= 0 && locals[i].depth > targetDepth; i--) emitByte(OP_POP);
}

static void expression(void);
static void declaration(void);
static void statement(void);
static void parsePrecedence(Precedence precedence);

static void number(bool canAssign) { (void)canAssign; emitConstant(NUMBER_VAL(strtod(parser.previous.start, NULL))); }
static void string(bool canAssign) {
  (void)canAssign;
  emitConstant(OBJ_VAL(constantString(parser.previous.start + 1, parser.previous.length - 2)));
}
static void namedVariable(bool canAssign) {
  Token name = parser.previous;
  int local = resolveLocal(&name);
  uint8_t arg = 0, getOp, setOp;
  if (local != -1) { getOp = OP_GET_LOCAL; setOp = OP_SET_LOCAL; }
  else { arg = identifierConstant(&name); getOp = OP_GET_GLOBAL; setOp = OP_SET_GLOBAL; }
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    if (local != -1 && locals[local].isConst) error("Cannot assign to a constant variable.");
    else if (local != -1) emitShort(setOp, (uint16_t)local);
    else emitBytes(setOp, arg);
  } else if (local != -1) emitShort(getOp, (uint16_t)local);
  else emitBytes(getOp, arg);
}
static void grouping(bool canAssign) { (void)canAssign; expression(); consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression."); }
static void unary(bool canAssign) {
  (void)canAssign; TokenType operatorType = parser.previous.type; parsePrecedence(PREC_UNARY);
  if (operatorType == TOKEN_BANG) emitByte(OP_NOT); else if (operatorType == TOKEN_MINUS) emitByte(OP_NEGATE);
}
static void binary(bool canAssign) {
  (void)canAssign; TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType); parsePrecedence((Precedence)(rule->precedence + 1));
  switch (operatorType) {
    case TOKEN_PLUS: emitByte(OP_ADD); break; case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY); break; case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
    case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break; case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
    case TOKEN_GREATER: emitByte(OP_GREATER); break; case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS: emitByte(OP_LESS); break; case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
    default: break;
  }
}
static void literal(bool canAssign) { (void)canAssign; if (parser.previous.type == TOKEN_FALSE) emitByte(OP_FALSE); else if (parser.previous.type == TOKEN_NIL) emitByte(OP_NIL); else emitByte(OP_TRUE); }
static void and_(bool canAssign) { (void)canAssign; int endJump = emitJump(OP_JUMP_IF_FALSE); emitByte(OP_POP); parsePrecedence(PREC_AND); patchJump(endJump); }
static void or_(bool canAssign) {
  (void)canAssign; int elseJump = emitJump(OP_JUMP_IF_FALSE); int endJump = emitJump(OP_JUMP);
  patchJump(elseJump); emitByte(OP_POP); parsePrecedence(PREC_OR); patchJump(endJump);
}

static ParseRule rules[TOKEN_EOF + 1] = {
  [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE}, [TOKEN_BANG] = {unary, NULL, PREC_NONE},
  [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY}, [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
  [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON}, [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
  [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON}, [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
  [TOKEN_MINUS] = {unary, binary, PREC_TERM}, [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
  [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR}, [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
  [TOKEN_IDENTIFIER] = {namedVariable, NULL, PREC_NONE}, [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
  [TOKEN_STRING] = {string, NULL, PREC_NONE}, [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
  [TOKEN_NIL] = {literal, NULL, PREC_NONE}, [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
  [TOKEN_AND] = {NULL, and_, PREC_AND}, [TOKEN_OR] = {NULL, or_, PREC_OR},
};
static ParseRule* getRule(TokenType type) { return &rules[type]; }
static void parsePrecedence(Precedence precedence) {
  advance(); bool canAssign = precedence <= PREC_ASSIGNMENT;
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (!prefixRule) { error("Expect expression."); return; }
  prefixRule(canAssign);
  while (precedence <= getRule(parser.current.type)->precedence) { advance(); getRule(parser.previous.type)->infix(canAssign); }
  if (canAssign && match(TOKEN_EQUAL)) error("Invalid assignment target.");
}
static void expression(void) { parsePrecedence(PREC_ASSIGNMENT); }

static void expressionStatement(void) { expression(); consume(TOKEN_SEMICOLON, "Expect ';' after expression."); emitByte(OP_POP); }
static void printStatement(void) { expression(); consume(TOKEN_SEMICOLON, "Expect ';' after value."); emitByte(OP_PRINT); }
static void varDeclaration(bool isConst) {
  consume(TOKEN_IDENTIFIER, "Expect variable name."); declareVariable(isConst);
  uint8_t global = scopeDepth == 0 ? identifierConstant(&parser.previous) : 0;
  if (match(TOKEN_EQUAL)) expression(); else emitByte(OP_NIL);
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration."); defineVariable(global, isConst);
}
static void block(void) {
  while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) declaration();
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void ifStatement(void) {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'."); expression(); consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  int thenJump = emitJump(OP_JUMP_IF_FALSE); emitByte(OP_POP); statement();
  int elseJump = emitJump(OP_JUMP); patchJump(thenJump); emitByte(OP_POP);
  if (match(TOKEN_ELSE)) statement();
  patchJump(elseJump);
}
static void whileStatement(void) {
  int loopStart = currentChunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'."); expression(); consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  int exitJump = emitJump(OP_JUMP_IF_FALSE); emitByte(OP_POP);
  Loop loop = {loopStart, scopeDepth, currentLoop}; currentLoop = &loop;
  statement(); emitLoop(loopStart); currentLoop = loop.enclosing;
  patchJump(exitJump); emitByte(OP_POP);
}
static void forStatement(void) {
  beginScope(); consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) { }
  else if (match(TOKEN_VAR)) varDeclaration(false);
  else if (match(TOKEN_CONST)) varDeclaration(true);
  else expressionStatement();

  int loopStart = currentChunk()->count; int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) { expression(); consume(TOKEN_SEMICOLON, "Expect ';' after loop condition."); exitJump = emitJump(OP_JUMP_IF_FALSE); emitByte(OP_POP); }

  int continueTarget = loopStart;
  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP); int incrementStart = currentChunk()->count;
    expression(); emitByte(OP_POP); consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
    emitLoop(loopStart); continueTarget = incrementStart; patchJump(bodyJump);
  }
  Loop loop = {continueTarget, scopeDepth, currentLoop}; currentLoop = &loop;
  statement(); emitLoop(continueTarget); currentLoop = loop.enclosing;
  if (exitJump != -1) { patchJump(exitJump); emitByte(OP_POP); }
  endScope();
}
static void continueStatement(void) {
  consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");
  if (!currentLoop) { error("Can't use 'continue' outside of a loop."); return; }
  emitLoopCleanup(currentLoop->scopeDepth); emitLoop(currentLoop->continueTarget);
}

static void switchStatement(void) {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'."); expression(); consume(TOKEN_RIGHT_PAREN, "Expect ')' after switch value.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");
  int* endJumps = NULL; int jumpCount = 0; int jumpCapacity = 0;
  while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) {
    bool isDefault = match(TOKEN_DEFAULT);
    if (!isDefault) consume(TOKEN_CASE, "Expect 'case' or 'default' in switch.");
    if (!isDefault) {
      emitByte(OP_DUP); expression(); consume(TOKEN_COLON, "Expect ':' after case value.");
      emitByte(OP_EQUAL); int nextCase = emitJump(OP_JUMP_IF_FALSE); emitByte(OP_POP);
      beginScope(); while (parser.current.type != TOKEN_CASE && parser.current.type != TOKEN_DEFAULT && parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) declaration(); endScope();
      if (jumpCount == jumpCapacity) { jumpCapacity = jumpCapacity < 4 ? 4 : jumpCapacity * 2; endJumps = (int*)realloc(endJumps, sizeof(int) * (size_t)jumpCapacity); }
      endJumps[jumpCount++] = emitJump(OP_JUMP); patchJump(nextCase); emitByte(OP_POP);
    } else {
      consume(TOKEN_COLON, "Expect ':' after 'default'."); beginScope();
      while (parser.current.type != TOKEN_CASE && parser.current.type != TOKEN_DEFAULT && parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) declaration();
      endScope();
    }
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after switch.");
  for (int i = 0; i < jumpCount; i++) patchJump(endJumps[i]);
  free(endJumps); emitByte(OP_POP);
}

static void statement(void) {
  if (match(TOKEN_PRINT)) printStatement();
  else if (match(TOKEN_IF)) ifStatement();
  else if (match(TOKEN_WHILE)) whileStatement();
  else if (match(TOKEN_FOR)) forStatement();
  else if (match(TOKEN_SWITCH)) switchStatement();
  else if (match(TOKEN_CONTINUE)) continueStatement();
  else if (match(TOKEN_LEFT_BRACE)) { beginScope(); block(); endScope(); }
  else expressionStatement();
}
static void synchronize(void) {
  parser.panicMode = false;
  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) { case TOKEN_CLASS: case TOKEN_FUN: case TOKEN_VAR: case TOKEN_CONST: case TOKEN_FOR: case TOKEN_IF: case TOKEN_WHILE: case TOKEN_PRINT: case TOKEN_SWITCH: return; default: break; }
    advance();
  }
}
static void declaration(void) {
  if (match(TOKEN_VAR)) varDeclaration(false); else if (match(TOKEN_CONST)) varDeclaration(true); else statement();
  if (parser.panicMode) synchronize();
}
bool compile(const char* source, Chunk* chunk) {
  compilingChunk = chunk; parser.hadError = false; parser.panicMode = false;
  locals = NULL; localCount = 0; localCapacity = 0; scopeDepth = 0; currentLoop = NULL;
  initScanner(source); advance(); while (!match(TOKEN_EOF)) declaration(); emitReturn(); free(locals); locals = NULL;
  return !parser.hadError;
}
