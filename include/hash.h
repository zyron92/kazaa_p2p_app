#ifndef __HASH_H__
#define __HASH_H__

//Data structure of element of hash table for super node
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "data_structure.h"

/*
 * The hash function to find hash index
 */
unsigned int hashFunction(unsigned int file_index);

/*
 * Perform a search to find the element associated to the file_index (key)
 */
HashTableElem *search(HashTableElem **table, unsigned int file_index);

/*
 * Add an element into hash table according to file_index (key), and the rest is data
 */
void insert_element(HashTableElem **table, unsigned int file_index,
                    unsigned int file_size, Child file_owner);

/*
 * Remove an element from hash table according to file_index (key)
 */
void delete_element(HashTableElem **table, unsigned int file_index);

/*
 * Free the hash table after the use
 */
void free_hashTable(HashTableElem **table);

#endif