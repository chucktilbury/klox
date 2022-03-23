/**
 * @file vm.h
 * @brief Define the interface to the virtual machine.
 *
 * @version 0.1
 * @date 2022-03-22
 *
 */
#ifndef _vm_h_
#define _vm_h_

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table strings;
    ObjString* initString;
    ObjUpvalue* openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;
    Obj* objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);

/*
 * These inlines help speed quite a bit...
 */
static inline void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

static inline Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static inline Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

#endif
