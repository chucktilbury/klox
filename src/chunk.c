/**
 * @file chunk.c
 * @brief Handle chunks of code for the virtual machine. The compiler writes
 * to these data strucutres and the VM reads from them,
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"

/**
 * @brief Set or reset the values in a chunk data structure.
 *
 * @param chunk - the chunk to init
 */
void initChunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

/**
 * @brief Free the memory associated with the chunk.
 *
 * @param chunk - the chunk to destroy
 *
 */
void freeChunk(Chunk* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/**
 * @brief The compiler writes to the chunk.
 *
 * @param chunk - the chunk to write to
 * @param byte - the byte to write to the chunk
 * @param line - line number from source code
 *
 */
void writeChunk(Chunk* chunk, uint8_t byte, int line)
{
    if(chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
                                 oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
                                  oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

/**
 * @brief Add a constant to the constant array in the chunk. This return value
 * is added to the instruction stream.
 *
 * @param chunk - the chunk to write to
 * @param value - the value to write
 * @return int - the index of the constant that was written.
 *
 */
int addConstant(Chunk* chunk, Value value)
{
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
    return chunk->constants.count - 1;
}
