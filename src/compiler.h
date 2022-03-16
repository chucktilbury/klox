#ifndef _compiler_h_
#define _compiler_h_

#include "object.h"
#include "vm.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();

#endif
