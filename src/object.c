/**
 * @file object.c
 * @brief Manipulate objects in the system. Most of the entities that are
 * referenced are handled in this module. (see values.c also)
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

/**
 * @brief Wrapper for allocateObject allows objects to be allocated by
 * object type.
 */
#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

/**
 * @brief Allocate an object and store it in the virtual machine. Program is
 * aborted if this operation fails.
 *
 * @param size - size of the object in bytes
 * @param type - enumerated type of the object
 * @return Obj* - object pointer
 *
 */
static Obj* allocateObject(size_t size, ObjType type)
{
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

/**
 * @brief Allocate a Bound Method object. Aborts program upon failure.
 *
 * @param receiver - ????
 * @param method - closure to bind
 * @return ObjBoundMethod* - pointer to the object.
 *
 */
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method)
{
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod,
                                         OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

/**
 * @brief Allocate a class object. Aborts program upon failure.
 *
 * @param name - class name used to store in the hash table
 * @return ObjClass* - pointer to the class object
 *
 */
ObjClass* newClass(ObjString* name)
{
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    return klass;
}

/**
 * @brief Allocate a closure object. Promotes upvalues. Aborts program upon
 * failure.
 *
 * @param function - pointer to compiled function
 * @return ObjClosure* - pointer to the new closure
 *
 */
ObjClosure* newClosure(ObjFunction* function)
{
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*,
                                     function->upvalueCount);
    for(int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

/**
 * @brief Create a new pointer to a function. Aborts program upon failure.
 *
 * @return ObjFunction* - pointer to the new function
 *
 */
ObjFunction* newFunction()
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

/**
 * @brief Create pointer to a new class instance.
 *
 * @param klass - pointer to the class
 * @return ObjInstance* - pointer to the new instance
 *
 */
ObjInstance* newInstance(ObjClass* klass)
{
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

/**
 * @brief Create a pointer to a new native function.
 *
 * @param function - the function to allocate
 * @return ObjNative* - pointer to the new function
 *
 */
ObjNative* newNative(NativeFn function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

/**
 * @brief Create a new string. Aborts the program upon failure.
 *
 * @param chars - raw pointer to an array of characters
 * @param length - length of the string (because it is not terminated)
 * @param hash - the hash value is used to de-duplicate the string table
 * @return ObjString* - pointer to the string object.
 *
 */
static ObjString* allocateString(char* chars, int length, uint32_t hash)
{
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();

    return string;
}

/**
 * @brief Create the hash value of the string. Used to store the string in
 * the hash table as well as de-duplicate the string table.
 *
 * @param key - the string to calculate the hash upon
 * @param length - the length of the string
 * @return uint32_t - the result of the calculation
 *
 */
static uint32_t hashString(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for(int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

/**
 * @brief Take ownership of the raw characters and create a string object
 * from them.
 *
 * @param chars - raw character buffer
 * @param length - length of the string
 * @return ObjString* - pointer to the new string object
 *
 */
ObjString* takeString(char* chars, int length)
{
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if(interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

/**
 * @brief Copy a raw buffer of characters into a string object.
 *
 * @param chars - buffer of characters
 * @param length - length of the buffer
 * @return ObjString* - pointer to resulting string
 *
 */
ObjString* copyString(const char* chars, int length)
{
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if(interned != NULL) {
        return interned;
    }

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

/**
 * @brief Create a new upvalue.
 * TODO: Research this and document what this really does.
 *
 * @param slot
 * @return ObjUpvalue*
 *
 */
ObjUpvalue* newUpvalue(Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

/**
 * @brief Print a function object to stdout.
 *
 * @param function - object to print
 *
 */
static void printFunction(ObjFunction* function)
{
    if(function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

/**
 * @brief Print various objects to stdout.
 *
 * @param value - object to print
 *
 */
void printObject(Value value)
{
    switch(OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_INSTANCE:
            printf("%s instance",
                   AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
    }
}
