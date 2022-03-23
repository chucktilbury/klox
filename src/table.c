/**
 * @file table.c
 * @brief Stand-alone hash table implementation. This is used to store
 * symbols as well as to deduplicate the string table.
 *
 * @version 0.1
 * @date 2022-03-20
 *
 */
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

/**
 * @brief This is used to know when to expand the hash table array.
 */
#define TABLE_MAX_LOAD 0.75

/**
 * @brief Initialize a hash table data structure.
 *
 * @param table - pointer to the table struct
 *
 */
void initTable(Table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

/**
 * @brief Free the memory associated with the hash table array. Note that the
 * caller is responsible for freeing the actual data in the hash table.
 *
 * @param table - hash table
 */
void freeTable(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

/**
 * @brief Find an entry slot in the hash table. If the entry does not exist,
 * then return a pointer to the slot where it can be placed.
 *
 * @param entries - hash table entry array
 * @param capacity - the size of the entry array
 * @param key - string that gives the key to be hashed
 *
 * @return Entry* - pointer to the entry slot
 */
static Entry* findEntry(Entry* entries, int capacity, ObjString* key)
{
    uint32_t index = key->hash & (capacity - 1);
    Entry* tombstone = NULL;

    for(;;) {
        Entry* entry = &entries[index];
        if(entry->key == NULL) {
            if(IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            }
            else {
                // We found a tombstone.
                if(tombstone == NULL) {
                    tombstone = entry;
                }
            }
        }
        else if(entry->key == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

/**
 * @brief Public interface function used to retrieve a hash table entry.
 *
 * @param table - hash table
 * @param key - name to look up
 * @param value - pointer to a generic Value to put the result into
 *
 * @return true - if the entry was found
 * @return false - if the entry was not found
 */
bool tableGet(Table* table, ObjString* key, Value* value)
{
    if(table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity,
                             key);
    if(entry->key == NULL) {
        return false;
    }

    *value = entry->value;
    return true;
}

/**
 * @brief Change the capacity of the table according to the current need. The
 * entries in the table have to be moved according to the new size as that
 * determins the location in the table.
 *
 * @param table - the hash table
 * @param capacity - new capacity
 */
static void adjustCapacity(Table* table, int capacity)
{
    Entry* entries = ALLOCATE(Entry, capacity);
    for(int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for(int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if(entry->key == NULL) {
            continue;
        }

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

/**
 * @brief Public interface to store an object into the hash table.
 *
 * @param table - the hash table
 * @param key - the string key to use
 * @param value - generic data object to store in the table
 *
 * @return true - if a new entry was stored in the table
 * @return false - if an entry was updated in the table
 */
bool tableSet(Table* table, ObjString* key, Value value)
{
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity,
                             key);
    bool isNewKey = entry->key == NULL;
    if(isNewKey && IS_NIL(entry->value)) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

/**
 * @brief Public interface to remove an entry from the hash table.
 *
 * @param table - the hash table
 * @param key - the key to use
 *
 * @return true - if the entry was found
 * @return false - if the entry was not in the table
 */
bool tableDelete(Table* table, ObjString* key)
{
    if(table->count == 0) {
        return false;
    }

    // Find the entry.
    Entry* entry = findEntry(table->entries, table->capacity,
                             key);
    if(entry->key == NULL) {
        return false;
    }

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

/**
 * @brief Public interface to copy the contents of one hash table to another
 * one.
 *
 * @param from - table to copy from
 * @param to - table to copy to
 */
void tableAddAll(Table* from, Table* to)
{
    for(int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if(entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

/**
 * @brief Public interface to specifically find a string object int he hash
 * table. This is used to de-duplicate the string table so that comparing one
 * string to another is simply comparing the pointer.
 *
 * @param table - the hash table
 * @param chars - contents of the string
 * @param length - length of the string (the string does not need to be terminated)
 * @param hash - hash value of the string
 *
 * @return ObjString* - if the string is found, return a pointer to the string
 * object, else return NULL.
 */
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash)
{
    if(table->count == 0) {
        return NULL;
    }

    uint32_t index = hash & (table->capacity - 1);
    for(;;) {
        Entry* entry = &table->entries[index];
        if(entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if(IS_NIL(entry->value)) {
                return NULL;
            }
        }
        else if(entry->key->length == length &&
                entry->key->hash == hash &&
                memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

// /**
//  * @brief Remove entries in the table for garbage collection.
//  *
//  * @param table - the hash table
//  */
// void tableRemoveWhite(Table* table)
// {
//     for(int i = 0; i < table->capacity; i++) {
//         Entry* entry = &table->entries[i];
//         if(entry->key != NULL && !entry->key->obj.isMarked) {
//             tableDelete(table, entry->key);
//         }
//     }
// }
//
