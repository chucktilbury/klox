/**
 * @file value.c
 * @brief Support generic data objects in the interpreter. Values are the
 * foundation for objects, which are the abstraction used to move defined
 * objects around the virtual machine.
 *
 * @version 0.1
 * @date 2022-03-22
 *
 */
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

/**
 * @brief Create a generic value data structure with no contents.
 *
 * @param array - value array to clear
 */
void initValueArray(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

/**
 * @brief Add an entry into the value array.
 *
 * @param array - the array to add into
 * @param value - the generic value to add
 */
void writeValueArray(ValueArray* array, Value value)
{
    if(array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values,
                                   oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

/**
 * @brief Destroy and clear the memory associated with a value array.
 *
 * @param array - the array to destroy
 */
void freeValueArray(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

/**
 * @brief For debugging. Print a representation of the value in the array to
 * stdout.
 *
 * @param value - the value to print
 */
void printValue(Value value)
{
#ifdef NAN_BOXING
    if(IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    }
    else if(IS_NIL(value)) {
        printf("nil");
    }
    else if(IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    }
    else if(IS_OBJ(value)) {
        printObject(value);
    }
#else

    switch(value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
    }
#endif
}

/**
 * @brief Check two numbers to see if they are equal. If they are not the
 * same type, then they cannot be equal.
 *
 * @param a - first value to compare
 * @param b - second value
 *
 * @return true - if the values are the same
 * @return false - if the values are different
 */
bool valuesEqual(Value a, Value b)
{
#ifdef NAN_BOXING
    if(IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    return a == b;
#else
    if(a.type != b.type) {
        return false;
    }
    switch(a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:
            return AS_OBJ(a) == AS_OBJ(b);
        default:
            return false; // Unreachable.
    }
#endif
}
