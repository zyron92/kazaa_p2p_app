#include "packet_handler.h"

/*
 * Initialization of a new packet data structure
 */
Packet *init_packet(unsigned int id, unsigned int msgType,
                    const unsigned char *info, unsigned int lengthInfo)
{
    //if the id is invalid (too big), we return a null pointer
    if (id > 0xffffffff) {
        fprintf(stderr, "ERROR: ID is invalid\n");
        return NULL;
    }

    //if the msgType is invalid,  we return a null pointer
    if (msgType > 0x00000052) { //bigger than the last message type possible
        fprintf(stderr, "ERROR: Msg Type is invalid\n");
        return NULL;
    }

    //If the creation of packet failed, we return a null pointer
    Packet *new_packet = calloc(1, sizeof(Packet));
    if (new_packet == NULL) {
        fprintf(stderr, "ERROR: Allocation of memory failed\n");
        return NULL;
    }

    new_packet->totalLength = 12; //by default the total length is 12bytes without info
    new_packet->id = id;
    new_packet->msgType = msgType;
    new_packet->info = NULL;

    //Initialise the info part of the packet
    if (lengthInfo > 0) {
        new_packet->totalLength += lengthInfo;
        new_packet->info = calloc(lengthInfo, sizeof(unsigned char));
        if (new_packet->info == NULL) {
            fprintf(stderr, "ERROR: Allocation of memory failed\n");
            free(new_packet);
            return NULL;
        }
        memcpy(new_packet->info, info, lengthInfo);
    }

    return new_packet;
}

/*
 * Initialization of a packet data structure from the socket
 */
Packet *read_packet(rio_t *readDescriptor)
{
    //Initialisation of internal reading buffer
    int nbReadBytes = 0;
    unsigned char buffer[4];

    //########## TOTAL LENGTH ##########
    nbReadBytes = Rio_readnb(readDescriptor, buffer, 4);
    //if number of read bytes not equal to what we expected, return a null pointer
    if (nbReadBytes == 0) {
        fprintf(stderr, "ERROR: No packet received\n");
        return NULL;
    }
    //if number of read bytes not equal to what we expected, return a null pointer
    if (nbReadBytes != 4) {
        fprintf(stderr, "ERROR: Total length of packet can't be read\n");
        return NULL;
    }
    unsigned int totalLength = bytesToInt(buffer, 0, 4);
    //if not enough bytes read, minimum is 12bytes
    if (totalLength < 12) {
        fprintf(stderr,
                "ERROR: Not enough bytes to be read; Minimum 12bytes\n");
        return NULL;
    }

    //########## ID ##########
    nbReadBytes = Rio_readnb(readDescriptor, buffer, 4);
    if (nbReadBytes != 4) {
        fprintf(stderr, "ERROR: ID of packet can't be read\n");
        return NULL;
    }
    unsigned int id = bytesToInt(buffer, 0, 4);

    //########## MSG TYPE ##########
    nbReadBytes = Rio_readnb(readDescriptor, buffer, 4);
    if (nbReadBytes != 4) {
        fprintf(stderr, "ERROR: Msg Type of packet can't be read\n");
        return NULL;
    }
    unsigned int msgType = bytesToInt(buffer, 0, 4);
    //if the msgType is invalid,  we return a null pointer
    if (msgType > 0x00000052) { //bigger than the last message type possible
        fprintf(stderr, "ERROR: Msg Type is invalid\n");
        return NULL;
    }

    //If the creation of packet failed, we return a null pointer
    Packet *new_packet = calloc(1, sizeof(Packet));
    if (new_packet == NULL) {
        fprintf(stderr, "ERROR: Allocation of memory failed\n");
        return NULL;
    }

    new_packet->totalLength = totalLength;
    new_packet->id = id;
    new_packet->msgType = msgType;
    new_packet->info = NULL;

    //Initialise the info part of the packet
    unsigned int lengthInfo = totalLength - 12;
    if (lengthInfo > 0) {
        new_packet->info = calloc(lengthInfo, sizeof(unsigned char));
        if (new_packet->info == NULL) {
            fprintf(stderr, "ERROR: Allocation of memory failed\n");
            free(new_packet);
            return NULL;
        }
        nbReadBytes = Rio_readnb(readDescriptor, new_packet->info, lengthInfo);
        if ((unsigned int) (nbReadBytes) != lengthInfo) {
            fprintf(stderr, "ERROR: Reading info part of packet failed\n");
            free(new_packet);
            return NULL;
        }
    }

    return new_packet;
}

/*
 * Free-ing packet data structure including info
 */
void free_packet(Packet *packetToFree)
{
    free(packetToFree->info);
    free(packetToFree);
}

/*
 * Converting a packet data structure to bytes in order to send them over socket
 * after copying
 */
unsigned char *packetToBytes(Packet *ptrPacket)
{
    if (ptrPacket == NULL) {
        return NULL;
    }

    unsigned char *new_bytes = calloc(ptrPacket->totalLength,
                                      sizeof(unsigned char));

    intToBytes(new_bytes, ptrPacket->totalLength, 0, 3);
    intToBytes(new_bytes, ptrPacket->id, 4, 7);
    intToBytes(new_bytes, ptrPacket->msgType, 8, 11);
    memcpy(new_bytes + 12, ptrPacket->info, ptrPacket->totalLength - 12);

    return new_bytes;
}

/*
 * Fill the array of byte, one by one, according to the element of type int
 */
void
intToBytes(unsigned char *bytesToFill, int element, unsigned int startPoint,
           unsigned int endPoint)
{
    unsigned int i;
    for (i = 0; i < endPoint - startPoint + 1; i++) {
        bytesToFill[startPoint + i] = (unsigned char) (
                (element >> (8 * (endPoint - startPoint - i))) & (0xFF));
    }
}

/*
 * Fill the array of byte, one by one, according to the element of type int
 * for 4 bytes fix and for string
 */
void intToString(unsigned char *bytesToFill, int element)
{
    unsigned int i;
    int k = 0;
    int result = 0;
    for (i = 0; i < 4; i++) {
        result = (element >> (8 * (4 - i))) & (0xFF);
        if (result != 0) {
            bytesToFill[k] = (unsigned char) (result);
            k++;
        }
    }
    bytesToFill[4] = '\0';
}

/*
 * Writing bytes to a file descriptor
 */
int write_packet(int output_fd, char *packetToWrite, unsigned int sizePacket)
{
    if (packetToWrite == NULL) {
        fprintf(stderr, "ERROR: zero byte to write\n");
        return ERR;
    }

    if (sizePacket != rio_writen(output_fd, packetToWrite, sizePacket)) {
        fprintf(stderr, "ERROR: writing to a socket failed\n");
        return ERR;
    }

    return OK;
}

/*
 * Converting some bytes in an array of char to a value in integer
 */
unsigned int bytesToInt(const unsigned char *readBytes,
                        int positionStartByte, unsigned int numBytes)
{
    //If number of bytes is more than the size of 'int',
    //we send an error code
    if (numBytes > sizeof(int)) {
        fprintf(stderr, "ERROR: Number of bytes to be converted to Int is \
too large\n");
        return ERR;
    }

    int currentInt = 0;
    unsigned int i;
    //we read one by one byte to produce a final value of int
    for (i = 0; i < numBytes; i++) {
        currentInt = (currentInt * 256) + (unsigned char)
                (readBytes[(positionStartByte + i)]);
    }

    return currentInt;
}