#ifndef _native_defs_h_
#define _native_defs_h_

#include "value.h"
#include "native.h"

typedef struct {
    const char* name;
    NativeFn func;
} NativeFunctions;

Value clockNative(int argCount, Value* args);

#endif