#ifndef __PACKET_HANDLER_H__
#define __PACKET_HANDLER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "csapp.h"
#include "msg_type.h"
#include "data_structure.h"

/*
 * Initialization of a new packet data structure
 */
Packet *init_packet(unsigned int id, unsigned int msgType,
                    const unsigned char *info, unsigned int lengthInfo);

/*
 * Initialization of a packet data structure from the socket
 */
Packet *read_packet(rio_t *readDescriptor);

/*
 * Free-ing packet data structure including info
 */
void free_packet(Packet *packetToFree);

/*
 * Converting a packet data structure to bytes in order to send them over socket
 * after copying
 */
unsigned char *packetToBytes(Packet *ptrPacket);

/*
 * Fill the array of byte, one by one, according to the element of type int
 */
void
intToBytes(unsigned char *bytesToFill, int element, unsigned int startPoint,
           unsigned int endPoint);

/*
 * Fill the array of byte, one by one, according to the element of type int
 * for 4 bytes fix and for string
 */
void intToString(unsigned char *bytesToFill, int element);

/*
 * Writing bytes to a file descriptor
 */
int write_packet(int output_fd, char *packetToWrite, unsigned int sizePacket);

/*
 * Converting some bytes in an array of char to a value in integer
 */
unsigned int bytesToInt(const unsigned char *readBytes,
                        int positionStartByte, unsigned int numBytes);

#endif
