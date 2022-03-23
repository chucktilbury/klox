/**
 * @file memory.c
 * @brief Memory manager for the language. Includes the garbage collector.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
    #include <stdio.h>
    #include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

/**
 * @brief Main memory allocation and free point in the system. Main support
 * for garbage collection. If the memory allocation fails, then this function
 * aborts the program. This function performs all memory allocation and free
 * operations.
 *
 * @param pointer - pointer to reallocate. Null creates a new pointer.
 * @param oldSize - used when downsizing a memory allocation
 * @param newSize - size of the new allocation
 *
 * @return void* - pointer to the allocated memory
 */
void* reallocate(void* pointer, size_t oldSize,
                 size_t newSize)
{
    vm.bytesAllocated += newSize - oldSize;
    if(newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif

        if(vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if(newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if(result == NULL) {
        exit(1);
    }
    return result;
}

/**
 * @brief Garbage collection. This function controls marking the object as gray.
 *
 * @param object - the object to mark
 */
void markObject(Obj* object)
{
    if(object == NULL) {
        return;
    }
    if(object->isMarked) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;

    if(vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack,
                                      sizeof(Obj*) * vm.grayCapacity);

        if(vm.grayStack == NULL) {
            exit(1);
        }
    }

    vm.grayStack[vm.grayCount++] = object;
}

/**
 * @brief Mark a value-type object.
 *
 * @param value - the value to mark
 */
void markValue(Value value)
{
    if(IS_OBJ(value)) {
        markObject(AS_OBJ(value));
    }
}

/**
 * @brief Mark an array-typed object as gray.
 *
 * @param array - the object to mark
 */
static void markArray(ValueArray* array)
{
    for(int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

/**
 * @brief Mark an object as black, which means that it is in use and will not
 * be deleted.
 *
 * @param object - the object to mark
 */
static void blackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch(object->type) {
        case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = (ObjBoundMethod*)object;
                markValue(bound->receiver);
                markObject((Obj*)bound->method);
                break;
            }
        case OBJ_CLASS: {
                ObjClass* klass = (ObjClass*)object;
                markObject((Obj*)klass->name);
                markTable(&klass->methods);
                break;
            }
        case OBJ_CLOSURE: {
                ObjClosure* closure = (ObjClosure*)object;
                markObject((Obj*)closure->function);
                for(int i = 0; i < closure->upvalueCount; i++) {
                    markObject((Obj*)closure->upvalues[i]);
                }
                break;
            }
        case OBJ_FUNCTION: {
                ObjFunction* function = (ObjFunction*)object;
                markObject((Obj*)function->name);
                markArray(&function->chunk.constants);
                break;
            }
        case OBJ_INSTANCE: {
                ObjInstance* instance = (ObjInstance*)object;
                markObject((Obj*)instance->klass);
                markTable(&instance->fields);
                break;
            }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

/**
 * @brief When the unreachable memory is reaped, then this is the function
 * that is called.
 *
 * @param object - object to free
 */
static void freeObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch(object->type) {
        case OBJ_BOUND_METHOD:
            FREE(ObjBoundMethod, object);
            break;
        case OBJ_CLASS: {
                ObjClass* klass = (ObjClass*)object;
                freeTable(&klass->methods);
                FREE(ObjClass, object);
                break;
            } // [braces]
        case OBJ_CLOSURE: {
                ObjClosure* closure = (ObjClosure*)object;
                FREE_ARRAY(ObjUpvalue*, closure->upvalues,
                           closure->upvalueCount);
                FREE(ObjClosure, object);
                break;
            }
        case OBJ_FUNCTION: {
                ObjFunction* function = (ObjFunction*)object;
                freeChunk(&function->chunk);
                FREE(ObjFunction, object);
                break;
            }
        case OBJ_INSTANCE: {
                ObjInstance* instance = (ObjInstance*)object;
                freeTable(&instance->fields);
                FREE(ObjInstance, object);
                break;
            }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
        case OBJ_STRING: {
                ObjString* string = (ObjString*)object;
                FREE_ARRAY(char, string->chars, string->length + 1);
                FREE(ObjString, object);
                break;
            }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
    }
}

/**
 * @brief Marking the roots of reachable objects.
 */
static void markRoots()
{
    for(Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    for(int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }

    for(ObjUpvalue* upvalue = vm.openUpvalues;
            upvalue != NULL;
            upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markTable(&vm.globals);
    markCompilerRoots();
    markObject((Obj*)vm.initString);
}

/**
 * @brief Find all of the object references.
 */
static void traceReferences()
{
    while(vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

/**
 * @brief Free all unmarked objects.
 */
static void sweep()
{
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while(object != NULL) {
        if(object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        }
        else {
            Obj* unreached = object;
            object = object->next;
            if(previous != NULL) {
                previous->next = object;
            }
            else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

/**
 * @brief Main interface. This is called to initiate the garbage collection.
 * This returns when all of the memory has been examined and kept or deleted.
 */
void collectGarbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated,
           vm.nextGC);
#endif
}

/**
 * @brief Free all of the objects at the end of the run.
 */
void freeObjects()
{
    Obj* object = vm.objects;
    while(object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}
