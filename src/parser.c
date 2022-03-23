
#include <stdio.h>

#include "common.h"
#include "scanner.h"
#include "parser.h"
#include "compiler.h"
#include "parser.h"
#include "expressions.h"

Parser parser;
extern ClassCompiler* currentClass; // defined in compiler.c

static void statement();

void errorAt(Token* token, const char* message)
{
    if(parser.panicMode) {
        return;
    }
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if(token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if(token->type == TOKEN_ERROR) {
        // Nothing.
    }
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if(constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

uint8_t identifierConstant(Token* name)
{
    return makeConstant(OBJ_VAL(copyString(name->start,
                                           name->length)));
}

static bool identifiersEqual(Token* a, Token* b)
{
    if(a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

static void block()
{
    while(!checkType(TOKEN_RIGHT_BRACE) && !checkType(TOKEN_EOF)) {
        declaration();
    }

    consumeToken(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consumeToken(TOKEN_LEFT_PAREN,
            "Expect '(' after function name.");
    if(!checkType(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if(current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while(matchToken(TOKEN_COMMA));
    }
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consumeToken(TOKEN_LEFT_BRACE,
            "Expect '{' before function body.");
    block();

    ObjFunction* function = endCompiler();

    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for(int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

void method()
{
    consumeToken(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if(parser.previous.length == 4 &&
            memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(type);
    emitBytes(OP_METHOD, constant);
}


static int resolveLocal(Compiler* compiler, Token* name)
{
    for(int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if(identifiersEqual(name, &local->name)) {
            if(local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index,
                      bool isLocal)
{
    int upvalueCount = compiler->function->upvalueCount;

    for(int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if(upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, Token* name)
{
    if(compiler->enclosing == NULL) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if(local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if(upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Token name)
{
    if(current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if(arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else if((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if(canAssign && matchToken(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else {
        emitBytes(getOp, (uint8_t)arg);
    }
}


static void declareVariable()
{
    if(current->scopeDepth == 0) {
        return;
    }

    Token* name = &parser.previous;
    for(int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if(local->depth != -1 &&
                local->depth < current->scopeDepth) {
            break;
        }

        if(identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

uint8_t parseVariable(const char* errorMessage)
{
    consumeToken(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if(current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser.previous);
}

static void markInitialized()
{
    if(current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth =
            current->scopeDepth;
}

void defineVariable(uint8_t global)
{
    if(current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void classDeclaration()
{
    consumeToken(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if(matchToken(TOKEN_LESS)) {
        consumeToken(TOKEN_IDENTIFIER, "Expect superclass name.");
        //variable(false);
        namedVariable(parser.previous, false);

        if(identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        beginScope();
        // this is added as a "synthetic" token so that there can be a symbol
        // to connect the compiled object to.
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    consumeToken(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while(!checkType(TOKEN_RIGHT_BRACE) && !checkType(TOKEN_EOF)) {
        method();
    }
    consumeToken(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if(classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void funDeclaration()
{
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration()
{
    uint8_t global = parseVariable("Expect variable name.");

    if(matchToken(TOKEN_EQUAL)) {
        expression();
    }
    else {
        emitByte(OP_NIL);
    }
    consumeToken(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement()
{
    expression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement()
{
    beginScope();
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if(matchToken(TOKEN_SEMICOLON)) {
        // No initializer.
    }
    else if(matchToken(TOKEN_VAR)) {
        varDeclaration();
    }
    else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if(!matchToken(TOKEN_SEMICOLON)) {
        expression();
        consumeToken(TOKEN_SEMICOLON,
                "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }

    if(!matchToken(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if(exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    endScope();
}

static void ifStatement()
{
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consumeToken(TOKEN_RIGHT_PAREN,
            "Expect ')' after condition."); // [paren]

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if(matchToken(TOKEN_ELSE)) {
        statement();
    }
    patchJump(elseJump);
}

static void printStatement()
{
    expression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement()
{
    if(current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if(matchToken(TOKEN_SEMICOLON)) {
        emitReturn();
    }
    else {
        if(current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        expression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement()
{
    int loopStart = currentChunk()->count;
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

static void synchronize()
{
    parser.panicMode = false;

    while(parser.current.type != TOKEN_EOF) {
        if(parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch(parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advanceToken();
    }
}

void consumeToken(TokenType type, const char* message)
{
    if(parser.current.type == type) {
        advanceToken();
        return;
    }

    errorAtCurrent(message);
}

bool checkType(TokenType type)
{
    return parser.current.type == type;
}

bool matchToken(TokenType type)
{
    if(!checkType(type)) {
        return false;
    }
    advanceToken();
    return true;
}

void advanceToken()
{
    parser.previous = parser.current;

    for(;;) {
        parser.current = scanToken();
        if(parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

static void statement()
{
    if(matchToken(TOKEN_PRINT)) {
        printStatement();
    }
    else if(matchToken(TOKEN_FOR)) {
        forStatement();
    }
    else if(matchToken(TOKEN_IF)) {
        ifStatement();
    }
    else if(matchToken(TOKEN_RETURN)) {
        returnStatement();
    }
    else if(matchToken(TOKEN_WHILE)) {
        whileStatement();
    }
    else if(matchToken(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    }
    else {
        expressionStatement();
    }
}

void declaration()
{
    if(matchToken(TOKEN_CLASS)) {
        classDeclaration();
    }
    else if(matchToken(TOKEN_FUN)) {
        funDeclaration();
    }
    else if(matchToken(TOKEN_VAR)) {
        varDeclaration();
    }
    else {
        statement();
    }

    if(parser.panicMode) {
        synchronize();
    }
}

