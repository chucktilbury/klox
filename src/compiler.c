#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "expressions.h"
#include "parser.h"

#ifdef DEBUG_PRINT_CODE
    #include "debug.h"
#endif

Compiler* current = NULL;
ClassCompiler* currentClass = NULL;
extern Parser parser; // defined in parser.c

void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if(offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

void emitReturn()
{
    if(current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    }
    else {
        emitByte(OP_NIL);
    }

    emitByte(OP_RETURN);
}

void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

void patchJump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;

    if(jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

void initCompiler(Compiler* compiler, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if(type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;

    if(type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    }
    else {
        local->name.start = "";
        local->name.length = 0;
    }
}

ObjFunction* endCompiler()
{
    emitReturn();
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if(!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
                         ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

void beginScope()
{
    current->scopeDepth++;
}

void endScope()
{
    current->scopeDepth--;

    while(current->localCount > 0 &&
            current->locals[current->localCount - 1].depth >
            current->scopeDepth) {

        if(current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        }
        else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

Chunk* currentChunk()
{
    return &current->function->chunk;
}

Token syntheticToken(const char* text)
{
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

ObjFunction* compile(const char* source)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advanceToken();

    while(!matchToken(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markCompilerRoots()
{
    Compiler* compiler = current;
    while(compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}
