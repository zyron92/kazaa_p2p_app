#ifndef __PACKET_HELPER_H__
#define __PACKET_HELPER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "csapp.h"
#include "packet_handler.h"
#include "msg_type.h"

/*
 * Send a generic packet to the socket
 */
int send_generic_packet(int socket_fd, unsigned int id, unsigned int msgType);

/*
 * Analyse the generic received packet from the socket
 */
int
receive_generic_packet(Packet *received_packet, unsigned int expectedMsgType,
                       unsigned int toFreePacket);

/*
 * ==========================================================================
 *  1) The objectives of the rest of functions are specified in the name of the functions
 *  2) Either send or analyse a specific message type of packet
 *  3) The utilised data structure is indicated too
 *  =========================================================================
 */

//info 2 bytes for port number for child to super or super to super
int send_hello_packet(unsigned int msgType, int socket_fd, unsigned int id,
                      unsigned int portNumber);

int
receive_hello_packet(unsigned int msgType, Packet *received_packet, unsigned int *portNumber,
                     unsigned int *id);

//info : for FILE_INFO & FILE_INFO_SHARE
//[2 bytes]+' ' : number of files (maximum num of files is 2 bytes) followed by character ' '
//[X bytes]+'\r' : file name with X bytes of its name followed by character '\r'
//[4 bytes]+'\t' : file size followed by character '\t'
//[16 bytes]+'\n' : ip address of the owner of the file followed by character '\n' if FILE_INFO_SHARE
//[2 bytes]+'\b' : port number of the owner of the file followed by character '\b' if FILE_INFO_SHARE
//The last two/four info will be multiplied according to the number of files
int send_file_info_packet(unsigned int msgType, int socket_fd, unsigned int id,
                          HashTableElem2 *all_files, unsigned int num_files);

int receive_file_info_packet(Packet *received_packet, unsigned int *num_files,
                             HashTableElem2 **all_files,
                             unsigned int expectedMsgType);

//info : for FILE_INFO & FILE_INFO_SHARE
//[2 bytes]+" " : number of files (maximum num of files is 2 bytes)
//[XX bytes]+'\r' : file index (equivalent to filename) of file 1
//The last 4 bytes will be multiplied according to the number of files
int
send_file_info_recv_packet(unsigned int msgType, int socket_fd, unsigned int id,
                           HashTableElem2 *all_files, unsigned int num_files);

int
receive_file_info_recv_packet(Packet *received_packet, unsigned int *num_files,
                              HashTableElem2 **all_files,
                              unsigned int expectedMsgType,
                              int toFreePacket);


//info :
//[X bytes] : file index (equivalent to filename) of file to search
int send_search_packet(int socket_fd, unsigned int id, Files file);

int receive_search_packet(Packet *received_packet, Files *file);

//info :
//[15 bytes] : ip address of child node
//[2 bytes] : port number of child node
int send_search_recv_packet(int socket_fd, unsigned int id, Child pChild);

int receive_search_recv_packet(Packet *received_packet, Child *pChild,
                               int toFreePacket);

//info :
//[X bytes] : file index (equivalent to filename) of file to download
int send_request_packet(int socket_fd, unsigned int id, Files file);

int receive_request_packet(Packet *received_packet, Files *file);

//info :
//[XX bytes] : the size depends on the size of file to download (we can retrieve this info from total length)
int send_request_recv_packet(int socket_fd, unsigned int id,
                             unsigned char *file_to_send,
                             unsigned int file_size);

int receive_request_recv_packet(Packet *received_packet, int toFreePacket);

#endif
