#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"
#include "native.h"

#include "vm_support.h"
VM vm;

/**
 * @brief Reset the frame stack. Called between VM runs.
 */
void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

/**
 * @brief Print a runtime error.
 *
 * @param format
 * @param ...
 */
void runtimeError(const char* format, ...)
{
    fprintf(stderr, "Runtime Error: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for(int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", // [minus]
                function->chunk.lines[instruction]);
        if(function->name == NULL) {
            fprintf(stderr, "script\n");
        }
        else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

/**
 * @brief Initialize the virtual machine and get it ready for a run.
 */
void initVM()
{
    resetStack();
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    vm.initString = NULL;
    vm.initString = copyString("init", 4);

    initNative();
}

/**
 * @brief Release the memory associated with a particular virtual machine run.
 */
void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
}

// /**
//  * @brief Push a value on to the runtime stack.
//  * TODO: Clarify what this stack actually does.
//  *
//  * @param value - the value to push on to the stack
//  */
// void push(Value value)
// {
//     *vm.stackTop = value;
//     vm.stackTop++;
// }

// /**
//  * @brief Remove a value from the runtime stack and return it.
//  *
//  * @return Value - return value
//  */
// Value pop()
// {
//     vm.stackTop--;
//     return *vm.stackTop;
// }

// /**
//  * @brief Peek at the stack at a given index.
//  *
//  * @param distance - index from top of the stack
//  *
//  * @return Value - return value
//  */
// Value peek(int distance)
// {
//     return vm.stackTop[-1 - distance];
// }

/**
 * @brief Call a closure.
 * TODO: Define the term "closure" better.
 *
 * @param closure - the closure to call
 * @param argCount - number of arguments on the value stack
 *
 * @return true - there was no runtime error
 * @return false - a runtime error occurred.
 */
bool call(ObjClosure* closure, int argCount)
{
    if(argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }

    if(vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

/**
 * @brief Call a callable object.
 *
 * @param callee - the object to call
 * @param argCount - number of args on the value stack
 *
 * @return true - if there was no runtime error
 * @return false - if a runtime error happened
 */
bool callValue(Value callee, int argCount)
{
    if(IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                    ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                    vm.stackTop[-argCount - 1] = bound->receiver;
                    return call(bound->method, argCount);
                }

            case OBJ_CLASS: {
                    ObjClass* klass = AS_CLASS(callee);
                    vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                    Value initializer;
                    if(tableGet(&klass->methods, vm.initString,
                                &initializer)) {
                        return call(AS_CLOSURE(initializer), argCount);
                    }
                    else if(argCount != 0) {
                        runtimeError("Expected 0 arguments but got %d.",
                                     argCount);
                        return false;
                    }
                    return true;
                }

            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);

            case OBJ_NATIVE: {
                    NativeFn native = AS_NATIVE(callee);
                    Value result = native(argCount, vm.stackTop - argCount);
                    vm.stackTop -= argCount + 1;
                    push(result);
                    return true;
                }
            default:
                break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

/**
 * @brief Invoke a class method from a closure.
 * TODO: Verify this
 *
 * @param klass - the class object
 * @param name - name of the class
 * @param argCount - number of args on the value stack
 *
 * @return true - if there was no runtime error
 * @return false - if there was a runtime error
 */
bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount)
{
    Value method;
    if(!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

/**
 * @brief Invoke a class method.
 *
 * @param name - name of the method
 * @param argCount - arg count on the value stack
 *
 * @return true
 * @return false
 */
bool invoke(ObjString* name, int argCount)
{
    Value receiver = peek(argCount);

    if(!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if(tableGet(&instance->fields, name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

/**
 * @brief Call a bound method.
 *
 * @param klass - class object
 * @param name - name of the bound object
 *
 * @return true - if there was no runtime error
 * @return false - if there was a runtime error
 */
bool bindMethod(ObjClass* klass, ObjString* name)
{
    Value method;
    if(!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

/**
 * @brief When a closure refers to a var that was defined outside of it, the
 * var needs to be caputured as an upvalue.
 *
 * @param local - the value to capture
 *
 * @return ObjUpvalue* - the captured upvalue
 */
ObjUpvalue* captureUpvalue(Value* local)
{
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while(upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if(upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if(prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    }
    else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

/**
 * @brief TODO: Define this
 *
 * @param last
 *
 */
void closeUpvalues(Value* last)
{
    while(vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

/**
 * @brief Add a method to the instruction stream.
 * TODO: define this.
 *
 * @param name
 *
 */
void defineMethod(ObjString* name)
{
    Value method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    pop();
}

/**
 * @brief Concatenate two string objects. Returns a third object with the
 * concatinated strings.
 */
void concatenate()
{
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}
