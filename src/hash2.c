#include "hash2.h"

/*
 * Simple hash function to find hash index (no collision handled)
 */
unsigned int hashFunction2(const char *file_index)
{
    unsigned int sum = 0;
    unsigned int i;
    for (i = 0; i < strlen(file_index); i++) {
        sum += file_index[i];
    }
    return sum % SIZE_HASHTABLE;
}

/*
 * Perform a search to find the element associated to the file_index (key)
 */
HashTableElem2 *search2(HashTableElem2 **table, const char *file_index)
{
    unsigned int index = hashFunction2(file_index);

    while (table[index] != NULL) {
        if (strcmp(table[index]->file_index, file_index) == 0) {
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
void insert_element2(HashTableElem2 **table, const char *file_index,
                     unsigned int file_size, Child file_owner)
{
    HashTableElem2 *new_item = calloc(1, sizeof(HashTableElem2));
    if (new_item == NULL) {
        return;
    }
    new_item->file_index = calloc(strlen(file_index) + 1, sizeof(char));
    strncpy(new_item->file_index, file_index, strlen(file_index));
    new_item->file_size = file_size;
    new_item->file_owner = file_owner;

    unsigned int index = hashFunction2(file_index);

    while (table[index] != NULL) {
        index++;
        index %= SIZE_HASHTABLE;
    }

    table[index] = new_item;
}

/*
 * Remove an element from hash table according to file_index (key)
 */
void delete_element2(HashTableElem2 **table, const char *file_index)
{
    HashTableElem2 *itemToRemove = search2(table, file_index);
    if (itemToRemove != NULL) {
        free(itemToRemove->file_index);
        free(itemToRemove);
    }
}

/*
 * Free the hash table after the use
 */
void free_hashTable2(HashTableElem2 **table)
{
    unsigned int i;
    for (i = 0; i < SIZE_HASHTABLE; i++) {
        if (table[i] != NULL) {
            free((table[i])->file_index);
            free(table[i]);
        }
    }
}