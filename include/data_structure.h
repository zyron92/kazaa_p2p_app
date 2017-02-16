#ifndef __DATA_STRUCTURE_H__
#define __DATA_STRUCTURE_H__

#include "csapp.h"

#define SIZE_HASHTABLE 100

/*
 * Structure of child node (information of child)
 */
typedef struct _child_info {
    char ipAddr[16]; //XXX.XXX.XXX.XXX\0 =>maximum 16bytes including the end delimiter of character '\0'
    unsigned int portNum;
} Child;

typedef Child Super;

/*
 * Structure of file
 */
typedef struct _file_info {
    char *file_index; //represents the name of file, eg : [file_index_in_string].txt after converting the filename to int
    unsigned int file_size;
} Files;

/*
 * Hash table structure for child's info
 */
typedef struct _hashTableElem {
    unsigned int file_index; //represents the id of the child
    unsigned int file_size;
    Child file_owner;
} HashTableElem;

/*
 * Hash table structure for super node's contents
 */
typedef struct _hashTableElem2 {
    char *file_index; //represents the name of file, eg : [file_index_in_string].txt after converting the filename to int
    unsigned int file_size;
    Child file_owner;
} HashTableElem2;


/*
 * Data structure of Packet
 */
typedef struct _packet {
    unsigned int totalLength; //4bytes
    unsigned int id; //4bytes
    unsigned int msgType; //4bytes
    unsigned char *info; //variable size
} Packet;

/*
 * Data structure of socket pool
 */
typedef struct _pool {
    fd_set active_read_set;
    fd_set ready_read_set;
    int clientfd[FD_SETSIZE];

    struct sockaddr_in clientAddress[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];

    int max_active_read_fd;

    int maxi;
    int num_ready;
} Pool;


/*
 * The table which contains all the files with the associated owner (child), which
 * are belong to a Super Node
 */
HashTableElem2 *superContents[SIZE_HASHTABLE];

/*
 * The table which contains all information of child port number for the file request
 * associated to the unique ID, so we reuse the same concept of hashtable but
 * the key (file_index) will actually be unique ID, and the file_size will be ignored.
 */
HashTableElem *childsPortNumber[SIZE_HASHTABLE];

/*
 * Information about neighbour super node
 */
Super neighbourSuper;
int neighbourSuperIsUpdated;

/*
 * The unique id
 */
unsigned int app_id;

/*
 * Information of the super node to connect for child
 */
unsigned int superNodePortNumber;
char *superNodeIp;

#endif
