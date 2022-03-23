#ifndef _parser_h_
#define _parser_h_

#include "common.h"
#include "value.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

void declaration();
bool checkType(TokenType type);
void advanceToken();
void consumeToken(TokenType type, const char* message);
bool matchToken(TokenType type);
uint8_t makeConstant(Value value);
uint8_t parseVariable(const char* errorMessage);
void defineVariable(uint8_t global);
uint8_t identifierConstant(Token* name);
void namedVariable(Token name, bool canAssign);

void errorAt(Token* token, const char* message);
#define error(message)  errorAt(&parser.previous, message)
#define errorAtCurrent(message) errorAt(&parser.current, message)

#endif