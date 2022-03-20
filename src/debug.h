/**
 * @file debug.h
 * @brief Header for the debug module.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#ifndef _debug_h_
#define _debug_h_

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif
