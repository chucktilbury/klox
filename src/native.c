#include <stdio.h>
#include <string.h>
#include <time.h>

#include "native_defs.h"
#include "vm.h"

static void defineNative(const char* name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static NativeFunctions natives[] = {
    {"clock", clockNative},
    {NULL, NULL}
};

void initNative() {

    for(int i = 0; natives[i].name != NULL; i++)
        defineNative(natives[i].name, natives[i].func);
}
