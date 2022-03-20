/**
 * @file native_defs.c
 * @brief Define all native functions in this module.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <time.h>

#include "native_defs.h"

/**
 * @brief <time.h> clock().
 *
 * @param argCount - number of parameters = 0
 * @param args - no arguments defined
 * @return Value - returns the clock value as defined in the C runtime.
 *
 */
Value clockNative(int argCount, Value* args)
{
    (void)argCount;
    (void)args;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

