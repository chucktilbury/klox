/**
 * @file memory.h
 * @brief Function and macro declarations for memory allocation and
 * garbage collection.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#ifndef _memory_h_
#define _memory_h_

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
                      sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

//void markObject(Obj* object);
//void markValue(Value value);
void freeObjects();
void collectGarbage();

#endif
