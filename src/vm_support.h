#ifndef _vm_support_h_
#define _vm_support_h_

#include "value.h"

void resetStack();
void runtimeError(const char* format, ...);

void initVM();
void freeVM();

bool call(ObjClosure* closure, int argCount);
bool callValue(Value callee, int argCount);
bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount);
bool invoke(ObjString* name, int argCount);
bool bindMethod(ObjClass* klass, ObjString* name);
ObjUpvalue* captureUpvalue(Value* local);
void closeUpvalues(Value* last);
void defineMethod(ObjString* name);
void concatenate();

#endif