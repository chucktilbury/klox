/**
 * @file native.c
 * @brief Define the functions that are used by the interpreter to create
 * native functions.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "native_defs.h"
#include "vm.h"

/**
 * @brief Create the native function object and register it with the system.
 *
 * @param name - the name as it will be seen in the source code
 * @param function - pointer to the handler
 *
 */
static void defineNative(const char* name,
                         NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

/**
 * @brief Definition of all native functions
 */
static NativeFunctions natives[] = {
    {"clock", clockNative, 0, NULL},
    {NULL, NULL, 0, NULL}
};

/**
 * @brief Iterate through the native function data structure and create all of
 * the functions.
 *
 */
void initNative()
{

    for(int i = 0; natives[i].name != NULL; i++) {
        defineNative(natives[i].name, natives[i].func);
    }
}
