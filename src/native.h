/**
 * @file native.h
 * @brief Declare the and initialize the native functions.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#ifndef _native_h_
#define _native_h_

typedef Value(*NativeFn)(int argCount, Value* args);

void initNative();

#endif