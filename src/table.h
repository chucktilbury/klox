/**
 * @file table.h
 * @brief Hash table public interface declarations.
 *
 * @version 0.1
 * @date 2022-03-22
 *
 */
#ifndef _table_h_
#define _table_h_

#include "common.h"
#include "value.h"

/**
 * @brief Hash table entry associates a string object to a generic value.
 */
typedef struct {
    ObjString* key;
    Value value;
} Entry;

/**
 * @brief General purpose hash table.
 */
typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash);

// support garbage collection
//void tableRemoveWhite(Table* table);
//void markTable(Table* table);

#endif
