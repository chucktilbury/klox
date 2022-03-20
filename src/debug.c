/**
 * @file debug.c
 * @brief Helper to use to debug the interpreter. This is not really used to
 * debug the code that is written in the target language.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

/**
 * @brief Top level entry point to disassemble a block of code.
 *
 * @param chunk - the chunk to operate on
 * @param name - a name to display that is associated with the chunk
 *
 */
void disassembleChunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);

    for(int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

/**
 * @brief Disassemble a constant instruction. These are instructions that
 * have an index into the values array associated with the chunk.
 *
 * @param name - the name of the instruction
 * @param chunk - the chunk that holds the instruction
 * @param offset - index of the instruction in the instruction array
 * @return int - new offset into the array of instructions
 *
 */
static int constantInstruction(const char* name,
                               Chunk* chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

/**
 * @brief Disassemble an INVOKE instruction. There are two different ones
 * that are disassembled in the same manner.
 *
 * @param name - name of the instruction for display
 * @param chunk - the chunk that holds the instruction
 * @param offset - index of the instruction in the instruction array
 * @return int - new offset into the instruction array
 *
 */
static int invokeInstruction(const char* name, Chunk* chunk,
                             int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

/**
 * @brief Disassemble instructions that have no operands.
 *
 * @param name - name of the instruction
 * @param offset - index of the instruction in the instruction array
 * @return int - new offset into the instruction array
 *
 */
static int simpleInstruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

/**
 * @brief Disassemble an instruction that has a single byte operand that is a
 * slot number for a global value.
 *
 * @param name - name of the instruction
 * @param chunk - the chunk that contains the data
 * @param offset - index of the instruction in the instruction array
 * @return int - new offset into the instruction array
 *
 */
static int byteInstruction(const char* name, Chunk* chunk,
                           int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

/**
 * @brief Disassemble an instruction that involves changing the instruction
 * pointer. The operand to this instruction is a 16 bit object.
 *
 * @param name - name of the instruction
 * @param sign - whether the jump increases the ip or decreases it
 * @param chunk - the chunk that contains the data
 * @param offset - index of the instruction in the instruction array
 * @return int - new offset into the instruction array
 *
 */
static int jumpInstruction(const char* name, int sign,
                           Chunk* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}

/**
 * @brief Process a single instruction from the instruction stream.
 *
 * @param chunk - the chunk that contains the data
 * @param offset - index of the instruction in the instruction array
 * @return int - new offset into the instruction array
 *
 */
int disassembleInstruction(Chunk* chunk, int offset)
{
    printf("%04d ", offset);
    if(offset > 0 &&
            chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    }
    else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch(instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk,
                                       offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", chunk,
                                       offset);
        case OP_SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", chunk,
                                       offset);
        case OP_GET_SUPER:
            return constantInstruction("OP_GET_SUPER", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk,
                                   offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_SUPER_INVOKE:
            return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
        case OP_CLOSURE: {
                offset++;
                uint8_t constant = chunk->code[offset++];
                printf("%-16s %4d ", "OP_CLOSURE", constant);
                printValue(chunk->constants.values[constant]);
                printf("\n");

                ObjFunction* function = AS_FUNCTION(
                                                chunk->constants.values[constant]);
                for(int j = 0; j < function->upvalueCount; j++) {
                    int isLocal = chunk->code[offset++];
                    int index = chunk->code[offset++];
                    printf("%04d      |                     %s %d\n",
                           offset - 2, isLocal ? "local" : "upvalue", index);
                }

                return offset;
            }
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CLASS:
            return constantInstruction("OP_CLASS", chunk, offset);
        case OP_INHERIT:
            return simpleInstruction("OP_INHERIT", offset);
        case OP_METHOD:
            return constantInstruction("OP_METHOD", chunk, offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
