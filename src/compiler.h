#ifndef _compiler_h_
#define _compiler_h_

#include "scanner.h"
#include "object.h"
#include "vm.h"

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperclass;
} ClassCompiler;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

void beginScope();
void endScope();
void patchJump(int offset);
void emitConstant(Value value);
void emitReturn();
int emitJump(uint8_t instruction);
void emitLoop(int loopStart);
void emitByte(uint8_t byte);
void emitBytes(uint8_t byte1, uint8_t byte2);

void markCompilerRoots();
Token syntheticToken(const char* text);

void initCompiler(Compiler* compiler, FunctionType type);
ObjFunction* endCompiler();
ObjFunction* compileSource(const char* source);

#define currentChunk() (&current->function->chunk)

#endif
