#include "packet_helper.h"

/*
 * Send packets generic function (include free the packet to be sent)
 */
static int send_generic_function(int socket_fd, Packet *packetToSend)
{
    unsigned char *packetToSendBytes = packetToBytes(packetToSend);
    if (packetToSendBytes == NULL) {
        return ERR;
    }

    //if number of sent bytes not equal to what we expected, print error
    if (write_packet(socket_fd, (char *) (packetToSendBytes),
                     packetToSend->totalLength) == ERR) {
        fprintf(stderr, "ERROR: sent bytes failed\n");
        free_packet(packetToSend);
        free(packetToSendBytes);
        return ERR;
    }

    free_packet(packetToSend);
    free(packetToSendBytes);
    return OK;
}

/*
 * Send a generic packet to the socket
 */
int send_generic_packet(int socket_fd, unsigned int id, unsigned int msgType)
{
    Packet *packetToSend = init_packet(id, msgType, NULL, 0);
    return send_generic_function(socket_fd, packetToSend);
}

/*
 * Analyse the generic received packet from the socket
 */
int
receive_generic_packet(Packet *received_packet, unsigned int expectedMsgType,
                       unsigned int toFreePacket)
{
    if (received_packet->totalLength < 12) {
        fprintf(stderr, "ERROR: not enough bytes to be read\n");
        if (toFreePacket == 1) {
            free_packet(received_packet);
        }
        return ERR;
    }

    if (received_packet->msgType != expectedMsgType) {
        fprintf(stderr, "ERROR: wrong packet received\n");
        if (toFreePacket == 1) {
            free_packet(received_packet);
        }
        return ERR;
    }

    if (toFreePacket == 1) {
        free_packet(received_packet);
    }
    return OK;
}

/*
 * ==========================================================================
 *  1) The objectives of the rest of functions are specified in the name of the functions
 *  2) Either send or analyse a specific message type of packet
 *  3) The utilised data structure is indicated too
 *  =========================================================================
 */

//info 2 bytes for port number for child to super or super to super
int send_hello_packet(unsigned int msgType, int socket_fd, unsigned int id,
                            unsigned int portNumber)
{
    //Port Number maximum size is 2 bytes
    unsigned char portNumberBytes[2];
    intToBytes(portNumberBytes, portNumber, 0, 1);

    Packet *packetToSend = init_packet(id, msgType, portNumberBytes, 2);
    return send_generic_function(socket_fd, packetToSend);
}

int
receive_hello_packet(unsigned int msgType, Packet *received_packet, unsigned int *portNumber,
                           unsigned int *id)
{
    if (receive_generic_packet(received_packet, msgType, 0) == ERR){
        free_packet(received_packet);
        return ERR;
    }

    if (received_packet->totalLength - 12 < 2) {
        fprintf(stderr, "ERROR: Info for port number can't be read\n");
        return ERR;
    }
    unsigned int receivedPortNumber = bytesToInt(received_packet->info, 0, 2);

    *portNumber = receivedPortNumber;
    *id = received_packet->id;

    free_packet(received_packet);
    return OK;
}

//info : for FILE_INFO & FILE_INFO_SHARE
//[2 bytes]+'\n' : number of files (maximum num of files is 2 bytes) followed by character ' '
//[X bytes]+'\r' : file name with X bytes of its name followed by character '\r'
//[4 bytes]+'\t' : file size followed by character '\t'
//[16 bytes]+'\n' : ip address of the owner of the file followed by character '\n' if FILE_INFO_SHARE
//[2 bytes]+'\b' : port number of the owner of the file followed by character '\b' if FILE_INFO_SHARE
//The last two/four info will be multiplied according to the number of files
int send_file_info_packet(unsigned int msgType, int socket_fd, unsigned int id,
                          HashTableElem2 *all_files, unsigned int num_files)
{
    if (num_files <= 0) {
        return OK;
    }

    unsigned char *fileInfoBytes = NULL;
    unsigned int currentSize = 0;
    currentSize += 2 + 1;
    fileInfoBytes = realloc(fileInfoBytes, currentSize * sizeof(char));

    intToBytes(fileInfoBytes, num_files, 0, 1);
    fileInfoBytes[2] = ' ';

    unsigned int filename_size;
    unsigned int current_ptr = 0;
    unsigned int i;
    for (i = 0; i < num_files; i++) {
        current_ptr = currentSize;

        filename_size = strlen(all_files[i].file_index);
        currentSize += (filename_size + 1 + 4 + 1);
        fileInfoBytes = realloc(fileInfoBytes, currentSize * sizeof(char));

        strncpy((char *) (fileInfoBytes + current_ptr),
                (char *) ((all_files[i]).file_index), filename_size);

        current_ptr = current_ptr + filename_size;
        fileInfoBytes[current_ptr] = '\r';

        current_ptr++;
        intToBytes(fileInfoBytes, (all_files[i]).file_size, current_ptr,
                   current_ptr + 3);
        fileInfoBytes[current_ptr + 4] = '\t';

        //if FILE_INFO_SHARE, add extra information on ip address and port number
        if (msgType == FILE_INFO_SHARE) {
            current_ptr = currentSize;
            currentSize += 20;
            fileInfoBytes = realloc(fileInfoBytes, currentSize * sizeof(char));
            strncpy((char *) (fileInfoBytes + current_ptr),
                    (all_files[i]).file_owner.ipAddr, 16);

            current_ptr += 16;
            fileInfoBytes[current_ptr] = '\n';

            current_ptr++;
            intToBytes(fileInfoBytes, (all_files[i]).file_owner.portNum,
                       current_ptr, current_ptr + 1);
            fileInfoBytes[current_ptr + 2] = '\b';
        }
    }

    Packet *packetToSend = init_packet(id, msgType, fileInfoBytes, currentSize);
    return send_generic_function(socket_fd, packetToSend);
}

static unsigned int findNextDelimiter(unsigned char *string, char delimiter)
{
    int currentPos = 0;

    while ((string + currentPos != NULL) &&
           (*(string + currentPos) != delimiter)) {
        currentPos++;
    }

    if (string + currentPos != NULL) {
        return currentPos;
    } else {
        return 0;
    }
}

int receive_file_info_packet(Packet *received_packet, unsigned int *num_files,
                             HashTableElem2 **all_files,
                             unsigned int expectedMsgType)
{
    if (receive_generic_packet(received_packet, expectedMsgType, 0) == ERR) {
        free_packet(received_packet);
        return ERR;
    }

    if (received_packet->totalLength - 12 < 2 + 1 + 1 + 1 + 4 + 1) {
        fprintf(stderr, "ERROR: Info for number of files info can't be read\n");
        return ERR;
    }
    *num_files = bytesToInt(received_packet->info, 0, 2);

    *all_files = calloc(*num_files, sizeof(HashTableElem2));

    unsigned int currentPos = 2 + 1;
    unsigned int nextPos = 2 + 1;
    unsigned int i;
    for (i = 0; i < *num_files; i++) {
        //filename
        nextPos = findNextDelimiter((received_packet->info) + currentPos, '\r');
        nextPos += currentPos;
        ((*all_files)[i]).file_index = calloc(nextPos - currentPos + 1,
                                              sizeof(char));
        strncpy((char *) ((*all_files)[i]).file_index,
                (char *) ((received_packet->info) + currentPos),
                nextPos - currentPos);
        currentPos = nextPos + 1;
        //filesize
        ((*all_files)[i]).file_size = bytesToInt(received_packet->info,
                                                 currentPos, 4);
        currentPos += 4 + 1;
        //ip info and port number for FILE INFO SHARE
        if (received_packet->msgType == FILE_INFO_SHARE) {
            //ip
            strncpy(((*all_files)[i]).file_owner.ipAddr,
                    (char *) ((received_packet->info) + currentPos), 16);
            currentPos += 16 + 1;
            //port
            ((*all_files)[i]).file_owner.portNum = bytesToInt(
                    received_packet->info, currentPos, 2);
            currentPos += 2 + 1;
        }
    }

    free_packet(received_packet);
    return OK;
}

//info : for FILE_INFO & FILE_INFO_SHARE
//[2 bytes]+" " : number of files (maximum num of files is 2 bytes)
//[XX bytes]+'\r' : file index (equivalent to filename) of file 1
//The last 4 bytes will be multiplied according to the number of files
int
send_file_info_recv_packet(unsigned int msgType, int socket_fd, unsigned int id,
                           HashTableElem2 *all_files, unsigned int num_files)
{
    if (num_files <= 0) {
        return OK;
    }

    unsigned char *fileInfoBytes = NULL;
    unsigned int currentSize = 0;
    currentSize += 2 + 1;
    fileInfoBytes = realloc(fileInfoBytes, currentSize * sizeof(char));

    intToBytes(fileInfoBytes, num_files, 0, 1);
    fileInfoBytes[2] = ' ';

    unsigned int filename_size;
    unsigned int current_ptr = 0;
    unsigned int i;

    for (i = 0; i < num_files; i++) {
        current_ptr = currentSize;
        filename_size = strlen(all_files[i].file_index);
        currentSize += (filename_size + 1);
        fileInfoBytes = realloc(fileInfoBytes, currentSize * sizeof(char));

        strncpy((char *) (fileInfoBytes + current_ptr),
                (char *) ((all_files[i]).file_index), filename_size);

        current_ptr = current_ptr + filename_size;
        fileInfoBytes[current_ptr] = '\r';
    }

    Packet *packetToSend = init_packet(id, msgType, fileInfoBytes, currentSize);
    return send_generic_function(socket_fd, packetToSend);
}

int
receive_file_info_recv_packet(Packet *received_packet, unsigned int *num_files,
                              HashTableElem2 **all_files,
                              unsigned int expectedMsgType,
                              int toFreePacket)
{
    if (receive_generic_packet(received_packet, expectedMsgType, 0) == ERR) {
        free_packet(received_packet);
        return ERR;
    }

    if (received_packet->totalLength - 12 < 2 + 1 + 1 + 1) {
        fprintf(stderr,
                "ERROR: Info for number of files info recv can't be read\n");
        return ERR;
    }

    *num_files = bytesToInt(received_packet->info, 0, 2);

    *all_files = calloc(*num_files, sizeof(HashTableElem2));

    unsigned int currentPos = 2 + 1;
    unsigned int nextPos = 2 + 1;
    unsigned int i;
    for (i = 0; i < *num_files; i++) {
        //filename
        nextPos = findNextDelimiter(((received_packet->info) + currentPos),
                                    '\r');
        nextPos += currentPos;

        ((*all_files)[i]).file_index = calloc(nextPos - currentPos + 1,
                                              sizeof(char));
        strncpy((char *) (((*all_files)[i]).file_index),
                (char *) ((received_packet->info) + currentPos),
                nextPos - currentPos);

        currentPos = nextPos + 1;
    }

    if (toFreePacket == 1) {
        free_packet(received_packet);
    }
    return OK;
}

//info :
//[X bytes] : file index (equivalent to filename) of file to search
int send_search_packet(int socket_fd, unsigned int id, Files file)
{
    unsigned char *fileInfoBytes = calloc(strlen(file.file_index),
                                          sizeof(char));
    strncpy((char *) fileInfoBytes, (char *) file.file_index,
            strlen(file.file_index));

    Packet *packetToSend = init_packet(id, SEARCH, fileInfoBytes,
                                       strlen(file.file_index));
    return send_generic_function(socket_fd, packetToSend);
}

int receive_search_packet(Packet *received_packet, Files *file)
{
    if (receive_generic_packet(received_packet, SEARCH, 0) == ERR) {
        free_packet(received_packet);
        return ERR;
    }

    file->file_index = calloc(received_packet->totalLength - 12 + 1,
                              sizeof(char));
    strncpy(file->file_index, (char *) (received_packet->info),
            received_packet->totalLength - 12);

    free_packet(received_packet);
    return OK;
}

//info :
//[15 bytes] : ip address of child node
//[2 bytes] : port number of child node
int send_search_recv_packet(int socket_fd, unsigned int id, Child pChild)
{
    unsigned char childInfo[18];
    memcpy(childInfo, pChild.ipAddr, 15);
    childInfo[15] = '\0';
    intToBytes(childInfo, pChild.portNum, 16, 17);

    Packet *packetToSend = init_packet(id, SEARCH_OK, childInfo, 18);
    return send_generic_function(socket_fd, packetToSend);
}

int receive_search_recv_packet(Packet *received_packet, Child *pChild,
                               int toFreePacket)
{
    if (receive_generic_packet(received_packet, SEARCH_OK, 0) == ERR) {
        free_packet(received_packet);
        return ERR;
    }

    if (received_packet->totalLength - 12 < 18) {
        fprintf(stderr,
                "ERROR: Info for file to search received info can't be read\n");
        return ERR;
    }
    memcpy(pChild->ipAddr, received_packet->info, 15);
    pChild->ipAddr[15] = '\0';
    pChild->portNum = bytesToInt(received_packet->info, 16, 2);

    if (toFreePacket == 1) {
        free_packet(received_packet);
    }
    return OK;
}

//info :
//[X bytes] : file index (equivalent to filename) of file to download
int send_request_packet(int socket_fd, unsigned int id, Files file)
{
    unsigned char *fileInfoBytes = calloc(strlen(file.file_index),
                                          sizeof(char));
    strncpy((char *) fileInfoBytes, (char *) file.file_index,
            strlen(file.file_index));

    Packet *packetToSend = init_packet(id, FILE_REQ, fileInfoBytes,
                                       strlen(file.file_index));
    return send_generic_function(socket_fd, packetToSend);
}

int receive_request_packet(Packet *received_packet, Files *file)
{
    if (receive_generic_packet(received_packet, FILE_REQ, 0) == ERR) {
        free_packet(received_packet);
        return ERR;
    }

    file->file_index = calloc(received_packet->totalLength - 12 + 1,
                              sizeof(char));
    strncpy(file->file_index, (char *) (received_packet->info),
            received_packet->totalLength - 12);

    free_packet(received_packet);
    return OK;
}

//info :
//[XX bytes] : the size depends on the size of file to download (we can retrieve this info from total length)
int send_request_recv_packet(int socket_fd, unsigned int id,
                             unsigned char *file_to_send,
                             unsigned int file_size)
{
    Packet *packetToSend = init_packet(id, FILE_REQ_OK, file_to_send,
                                       file_size);
    return send_generic_function(socket_fd, packetToSend);
}

int receive_request_recv_packet(Packet *received_packet, int toFreePacket)
{
    if (receive_generic_packet(received_packet, FILE_REQ_OK, 0) == ERR) {
        free_packet(received_packet);
        return ERR;
    }

    if (toFreePacket == 1) {
        free_packet(received_packet);
    }
    return OK;
}


