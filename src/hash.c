#include "hash.h"

/*
 * The hash function to find hash index
 */
unsigned int hashFunction(unsigned int file_index)
{
    return file_index % SIZE_HASHTABLE;
}

/*
 * Perform a search to find the element associated to the file_index (key)
 */
HashTableElem *search(HashTableElem **table, unsigned int file_index)
{
    unsigned int index = hashFunction(file_index);

    while (table[index] != NULL) {
        if (table[index]->file_index == file_index) {
            return table[index];
        }

        index++;
        index %= SIZE_HASHTABLE;
    }

    return NULL;
}

/*
 * Add an element into hash table according to file_index (key), and the rest is data
 */
void insert_element(HashTableElem **table, unsigned int file_index,
                    unsigned int file_size, Child file_owner)
{
    HashTableElem *new_item = calloc(1, sizeof(HashTableElem));
    if (new_item == NULL) {
        return;
    }
    new_item->file_index = file_index;
    new_item->file_size = file_size;
    new_item->file_owner = file_owner;

    unsigned int index = hashFunction(file_index);

    while (table[index] != NULL) {
        index++;
        index %= SIZE_HASHTABLE;
    }

    table[index] = new_item;
}

/*
 * Remove an element from hash table according to file_index (key)
 */
void delete_element(HashTableElem **table, unsigned int file_index)
{
    HashTableElem *itemToRemove = search(table, file_index);
    free(itemToRemove);
}

/*
 * Free the hash table after the use
 */
void free_hashTable(HashTableElem **table)
{
    unsigned int i;
    for (i = 0; i < SIZE_HASHTABLE; i++) {
        if (table[i] != NULL) {
            free(table[i]);
        }
    }
}