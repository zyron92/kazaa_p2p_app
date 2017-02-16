#ifndef __HASH2_H__
#define __HASH2_H__

//Data structure of element of hash table for super node
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "data_structure.h"

/*
 * Simple hash function to find hash index (no collision handled)
 */
unsigned int hashFunction2(const char *file_index);

/*
 * Perform a search to find the element associated to the file_index (key)
 */
HashTableElem2 *search2(HashTableElem2 **table, const char *file_index);

/*
 * Add an element into hash table according to file_index (key), and the rest is data
 */
void insert_element2(HashTableElem2 **table, const char *file_index,
                     unsigned int file_size, Child file_owner);

/*
 * Remove an element from hash table according to file_index (key)
 */
void delete_element2(HashTableElem2 **table, const char *file_index);

/*
 * Free the hash table after the use
 */
void free_hashTable2(HashTableElem2 **table);

#endif