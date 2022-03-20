/**
 * @file native_defs.h
 * @brief Function protos for native functions that are defined in the
 * corresponding C source code.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#ifndef _native_defs_h_
#define _native_defs_h_

#include "value.h"
#include "native.h"

/**
 * @brief Data structure that defines the interface from the native function
 * definitions to the interpreter.
 */
typedef struct {
    const char* name;
    NativeFn func;
    int count;
    Value* args;
} NativeFunctions;

Value clockNative(int argCount, Value* args);

#endif