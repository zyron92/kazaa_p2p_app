#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "msg_type.h"
#include "data_structure.h"
#include "packet_handler.h"

/*
 * Scan folder for files in the directory of data/ or other directories
 */
HashTableElem2 *scan_folder(const char *foldername, int *numFiles);

/*
 * Save data in a file in directory download/
 */
int save_data(Packet *readPacket, const char *filename);

/*
 * Reading the whole file from data/ and send them as strings
 */
char *read_whole_file(const char *filename, int *filesize);

#endif