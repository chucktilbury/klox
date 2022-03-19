#ifndef _native_h_
#define _native_h_

typedef Value(*NativeFn)(int argCount, Value* args);

void initNative();

#endif