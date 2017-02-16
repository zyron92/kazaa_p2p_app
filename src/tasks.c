#include "tasks.h"

/*
 * Initialisation of socket pool
 */
void init_pool(int listen_fd, Pool *p)
{
    //Initialisation of informations for select
    FD_ZERO(&(p->active_read_set));
    FD_ZERO(&(p->ready_read_set));
    FD_SET(listen_fd, &(p->active_read_set));
    p->max_active_read_fd = listen_fd;

    //Initialisation of all client fd
    //by default, negative number for non-connected socket
    unsigned int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
    }

    p->maxi = -1;
    p->num_ready = 0;
}

/*
 * Add a new client to the Pool
 */
void add_client(int connfd, struct sockaddr_in clientAddress, Pool *p)
{
    p->num_ready--;

    // Find an available slot
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (p->clientfd[i] < 0) {
            // Add connected descriptor to the pool & initialise reading buffer
            p->clientfd[i] = connfd;
            p->clientAddress[i] = clientAddress;
            Rio_readinitb(&p->clientrio[i], connfd);

            //Add the descriptor to active read descriptor set
            FD_SET(connfd, &(p->active_read_set));

            // Update max descriptor
            if (connfd > p->max_active_read_fd) {
                p->max_active_read_fd = connfd;
            }
            if (i > p->maxi) {
                p->maxi = i;
            }
            break;
        }
    }
    // Couldn't find an empty slot
    if (i == FD_SETSIZE) {
        fprintf(stderr, "ERROR: add_client error: Too many clients\n");
    }
}

/*
 * Close the socket and clear from the pool
 */
static void close_socket(int fd, int *client_fd, fd_set *active_read_set)
{
    close(fd);
    FD_CLR(fd, active_read_set);
    *client_fd = -1;
}

/*
 * Check added clients of child and do a task for each of them is they are active
 */
void check_clients_child(Pool *p)
{
    int i, connfd;
    rio_t rio;
    Packet *receivedPacket;

    for (i = 0; (i <= p->maxi) && (p->num_ready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];
        //If we receive something from the sockets, we do a task according to
        //the received packet
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_read_set))) {
            p->num_ready--;
            receivedPacket = read_packet(&rio);
            if (receivedPacket != NULL) {
                do_task_child(receivedPacket, connfd);
                close_socket(connfd, &(p->clientfd[i]), &p->active_read_set);
            } else {
                close_socket(connfd, &(p->clientfd[i]), &p->active_read_set);
            }
        }
    }
}

/*
 * Check added clients of super and do a task for each of them is they are active
 */
void check_clients_super(Pool *p)
{
    int i, connfd;
    rio_t rio;
    Packet *receivedPacket;
    for (i = 0; (i <= p->maxi) && (p->num_ready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];
        //If we receive something from the sockets, we do a task according to
        //the received packet
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_read_set))) {
            p->num_ready--;
            receivedPacket = read_packet(&rio);
            if (receivedPacket != NULL) {
                do_task_super(receivedPacket, connfd, p->clientAddress[i]);
                close_socket(connfd, &(p->clientfd[i]), &p->active_read_set);
            } else {
                close_socket(connfd, &(p->clientfd[i]), &p->active_read_set);
            }
        }
    }
}

/*
 * Parse the get command into filenames
 */
static int getFilenames(char *buffer, int sizeBytes, char **filenameToSearch,
                        char **filenameToSave)
{
    //Informations by default
    *filenameToSearch = NULL;
    *filenameToSave = NULL;

    if (buffer == NULL) {
        fprintf(stderr, "ERROR: no get files detected\n");
        return ERR;
    }

    /*
     * Verification if there is "get "
     */
    buffer[sizeBytes] = '\0';
    char *bufferAfterGet = NULL;
    unsigned int bufferLength = sizeBytes;
    if ((bufferLength >= 5 && (strncmp(buffer, "get ", 4) == 0))) {
        bufferAfterGet = buffer + 4;
    } else {
        fprintf(stderr, "ERROR: get files format invalid\n");
        return ERR;
    }

    /*
     * Information on the filename to save
     */
    char *afterSeparatorFile = strstr(bufferAfterGet, " ");
    unsigned int afterSepFileLength;
    if (afterSeparatorFile != NULL) {
        afterSeparatorFile++;
        afterSepFileLength = strlen(afterSeparatorFile);
        *filenameToSave = calloc(afterSepFileLength, sizeof(char));
        strncpy(*filenameToSave, afterSeparatorFile, afterSepFileLength);
        (*filenameToSave)[afterSepFileLength - 1] = '\0';
    } else {
        fprintf(stderr, "ERROR: get files format invalid\n");
        return ERR;
    }

    /*
     * Information on the filename to search
     */
    *filenameToSearch = calloc((sizeBytes - 4 - 1 - afterSepFileLength + 1),
                               sizeof(char));
    strncpy(*filenameToSearch, buffer + 4,
            sizeBytes - 4 - 1 - afterSepFileLength);
    (*filenameToSearch)[sizeBytes - 4 - 1 - afterSepFileLength] = '\0';

    return OK;
}

/*
 * Download the requested file after got the information from the supernode
 */
void do_download(const char *buffer, int sizeBytes)
{
    char usefulBuffer[sizeBytes + 1];
    strncpy(usefulBuffer, buffer, sizeBytes);

    /*
     * -------------------------------------------------------------------------
     * Parsing the command received on stdin
     * -------------------------------------------------------------------------
     */
    char *filenameToSearch;
    char *filenameToSave;
    if (getFilenames(usefulBuffer, sizeBytes, &filenameToSearch,
                     &filenameToSave) == ERR) {
        fprintf(stderr, "ERROR: get failed, verify filenames\n");
        return;
    }

    /*
     * -------------------------------------------------------------------------
     * Search query on Super Node
     * -------------------------------------------------------------------------
     */
    //Connecting to supernode to search for the owner of requested file
    int supernode_fd = client_connecting(superNodeIp, superNodePortNumber);
    Child pChild;
    //send file query info
    Files fileToSearchFor;
    fileToSearchFor.file_index = calloc(strlen(filenameToSearch) + 1,
                                        sizeof(char));
    strncpy(fileToSearchFor.file_index, filenameToSearch,
            strlen(filenameToSearch));
    if (send_search_packet(supernode_fd, app_id, fileToSearchFor) == ERR) {
        fprintf(stderr, "ERROR: failed to send file query info packet\n");
        return;
    }
    //receive reply file query info by listening to any incoming packet
    Packet *reply_file_query_packet;
    if (wait_for_reply_using_select(supernode_fd, &reply_file_query_packet) ==
        ERR) {
        fprintf(stderr, "ERROR: failed to receive any packet\n");
        return;
    }
    //verify received packet for file query
    if (receive_search_recv_packet(reply_file_query_packet, &pChild, 0) ==
        ERR) {
        if (receive_generic_packet(reply_file_query_packet, SEARCH_ERR, 0) ==
            ERR) {
            fprintf(stderr,
                    "ERROR: failed to receive the queried file\n");
            return;
        } else {
            fprintf(stderr,
                    "ERROR: querying file failed. Try again.\n");
            return;
        }
    }

    /*
     * -------------------------------------------------------------------------
     * Request & Download from the child for the requested file
     * -------------------------------------------------------------------------
     */
    //Connecting to the child on question to request & download requested file
    int child_fd = client_connecting(pChild.ipAddr, pChild.portNum);
    //send file request info
    if (send_request_packet(child_fd, app_id, fileToSearchFor) == ERR) {
        fprintf(stderr, "ERROR: failed to send file request info packet\n");
        return;
    }
    //receive reply file request info by listening to any incoming packet
    Packet *reply_file_request_packet;
    if (wait_for_reply_using_select(child_fd, &reply_file_request_packet) ==
        ERR) {
        fprintf(stderr, "ERROR: failed to receive any packet\n");
        return;
    }
    //verify received packet for file request
    if (receive_request_recv_packet(reply_file_request_packet, 0) == ERR) {
        if (receive_generic_packet(reply_file_request_packet, FILE_REQ_ERR,
                                   0) ==
            ERR) {
            fprintf(stderr,
                    "ERROR: failed to receive the requested file\n");
            return;
        } else {
            fprintf(stderr,
                    "ERROR: requesting file failed. Try again.\n");
            return;
        }
    }

    //save received file into download folder
    save_data(reply_file_request_packet, filenameToSave);
    fprintf(stderr, "==> FILE RECEIVED\n");
    free_packet(reply_file_request_packet);
}

/*
 * Do a task for super according to received packet from the listening socket
 */
void do_task_super(Packet *received_packet, int connfd,
                   struct sockaddr_in clientAddress)
{
    int msgType = (int) received_packet->msgType;

    switch (msgType) {
        case CHILD_HELLO : {
            //Check the portnumber of the child for the file request
            unsigned int portNumber;
            unsigned int id;
            if (receive_hello_packet(CHILD_HELLO, received_packet, &portNumber,
                                     &id) ==
                ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive the Child Hello.\n");
                return;
            }

            //Store client input into hash table(ip address and port number for the file request later)
            Child pchild;
            char *ipAdress;
            int dummy_portNumber;
            if (get_connected_client_info(&clientAddress, &ipAdress,
                                          &dummy_portNumber) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to get child ip info.\n");
                return;
            }
            strncpy(pchild.ipAddr, ipAdress, strlen(ipAdress));
            pchild.portNum = portNumber;
            //avoid duplication before inserting
            if (search(childsPortNumber, id) == NULL) {
                insert_element(childsPortNumber, id, 0, pchild);
            }

            //reply client hello over the socket
            if (send_generic_packet(connfd, app_id, SUPER_HELLO) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to send Super Hello\n");
                return;
            }
            break;
        }
        case SUPER2SUPER_HELLO : {
            //Check the listening portnumber of the neighbour super for the any file info sharing
            unsigned int portNumber;
            unsigned int id;
            if (receive_hello_packet(SUPER2SUPER_HELLO, received_packet,
                                     &portNumber, &id) ==
                ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive the Super2Super Hello.\n");
                return;
            }
            //Update neighbour super node info for easy access
            char *ipAdress;
            int dummy_portNumber;
            if (get_connected_client_info(&clientAddress, &ipAdress,
                                          &dummy_portNumber) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to get super ip info.\n");
                return;
            }
            strncpy(neighbourSuper.ipAddr, ipAdress, strlen(ipAdress));
            neighbourSuper.portNum = portNumber;
            neighbourSuperIsUpdated = 1;
            break;
        }
        case FILE_INFO : {
            unsigned int child_id = received_packet->id;

            //Check the received files infos
            unsigned int numberFiles;
            HashTableElem2 *received_files;

            if (receive_file_info_packet(received_packet, &numberFiles,
                                         &received_files, FILE_INFO) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive the FILE_INFO.\n");
                //send file info fail
                send_generic_packet(connfd, app_id, FILE_INFO_ERR);
                return;
            }

            //Store files infos with its owner into hash table for the file request later
            HashTableElem *owner = search(childsPortNumber, child_id);
            //check if the owner exits
            if (owner == NULL) {
                fprintf(stderr,
                        "ERROR: no child (owner) info for these files\n");
            }

            unsigned int i;
            for (i = 0; i < numberFiles; i++) {
                //avoid duplication before inserting
                if (search2(superContents, received_files[i].file_index) ==
                    NULL) {
                    received_files[i].file_owner = owner->file_owner; //copy for neighbour super file info sharing
                    insert_element2(superContents, received_files[i].file_index,
                                    received_files[i].file_size,
                                    owner->file_owner);
                }
            }

            //reply file info
            if (send_file_info_recv_packet(FILE_INFO_OK, connfd, app_id,
                                           received_files, numberFiles) ==
                ERR) {
                fprintf(stderr,
                        "ERROR: failed to send super Hello\n");
                return;
            }

            //send info to the other host & waiting for reply if neighbour is set
            if (neighbourSuperIsUpdated == 1) {
                //new tcp connection
                int neighbourSupernode_fd = client_connecting(
                        neighbourSuper.ipAddr, neighbourSuper.portNum);
                //send files info
                if (send_file_info_packet(FILE_INFO_SHARE,
                                          neighbourSupernode_fd, app_id,
                                          received_files,
                                          numberFiles) == ERR) {
                    fprintf(stderr,
                            "ERROR: failed to share File Info with other super node (SEND) "
                                    "- (may also caused by self-sharing which is not supposed to do), "
                                    "But this node will continue\n");
                    return;
                }

                //receive reply file info share by listening to any incoming packet
                Packet *reply_file_info_packet;
                if (wait_for_reply_using_select(neighbourSupernode_fd,
                                                &reply_file_info_packet) ==
                    ERR) {
                    fprintf(stderr,
                            "ERROR: failed to receive any packet from neighbour super node "
                                    "(may also caused by self-sharing which is not supposed to do), "
                                    "But this super node will continue.\n");
                    return;
                }

                HashTableElem2 *receivedReplyFiles;
                unsigned int numFilesReplied;
                //verify reply file info share
                if (receive_file_info_recv_packet(reply_file_info_packet,
                                                  &numFilesReplied,
                                                  &receivedReplyFiles,
                                                  FILE_INFO_SHARE_OK, 0) ==
                    ERR) {
                    if (receive_generic_packet(reply_file_info_packet,
                                               FILE_INFO_SHARE_ERR,
                                               0) ==
                        ERR) {
                        fprintf(stderr,
                                "ERROR: failed to receive a correct File Info Share Reply. But this super node will continue.\n");
                    } else {
                        fprintf(stderr,
                                "ERROR: File Info Share sharing failed with this neighbour super node. But this super node will continue.\n");
                    }
                }
            }
            break;
        }
        case SEARCH : {
            //check the received received packets for File Query
            Files fileToQuery;
            if (receive_search_packet(received_packet, &fileToQuery) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive the SEARCH QUERY.\n");
                //send file query fail
                send_generic_packet(connfd, app_id, SEARCH_ERR);
                return;
            }

            //search for file requested
            HashTableElem2 *elemFound;
            if ((elemFound = search2(superContents, fileToQuery.file_index)) ==
                NULL) {
                fprintf(stderr,
                        "ERROR: no matched file to query.\n");
                //send file query fail
                send_generic_packet(connfd, app_id, SEARCH_ERR);
                return;
            }

            //Reply search query
            if (send_search_recv_packet(connfd, app_id,
                                        elemFound->file_owner) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to send the owner of the matched file.\n");
                //send file query fail
                return;
            }
            break;
        }
        case FILE_INFO_SHARE : {
            //check the received received packets for File Info Share
            unsigned int number_files;
            HashTableElem2 *all_files;
            if (receive_file_info_packet(received_packet, &number_files,
                                         &all_files, FILE_INFO_SHARE) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive the FILE INFO SHARE.\n");
                //send file query fail
                send_generic_packet(connfd, app_id, FILE_INFO_SHARE_ERR);
                return;
            }

            //insert all the shared new elements by other super node
            //avoid duplication before inserting
            unsigned int i;
            for (i = 0; i < number_files; i++) {
                if (search2(superContents, all_files[i].file_index) == NULL) {
                    insert_element2(superContents, all_files[i].file_index,
                                    all_files[i].file_size,
                                    all_files[i].file_owner);
                }
            }

            //Reply file info share
            if (send_file_info_recv_packet(FILE_INFO_SHARE_OK, connfd, app_id,
                                           all_files, number_files) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to send the reply of file info share.\n");
                return;
            }
            break;
        }
        default:
            fprintf(stderr,
                    "ERROR: this packet should not be received! It will be ignored.\n");
            break;
    }
}


/*
 * Do a task for child according to received packet from the listening socket
 * Only for the requesting and
 */
void do_task_child(Packet *received_packet, int connfd)
{
    unsigned int msgType = received_packet->msgType;

    switch (msgType) {
        case FILE_REQ: {
            //Check the info of the requested file
            Files file;
            if (receive_request_packet(received_packet, &file) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive the queried file. Error message sent.\n");
                //send request fail
                send_generic_packet(connfd, app_id, FILE_REQ_ERR);
                return;
            }

            //send the content over the socket
            unsigned int filesize;
            char *file_contents = read_whole_file(file.file_index,
                                                  (int *) (&filesize));
            if (send_request_recv_packet(connfd, app_id,
                                         (unsigned char *) (file_contents),
                                         filesize) == ERR) {
                fprintf(stderr,
                        "ERROR: failed to send file contents packet\n");
                return;
            }
            break;
        }
        default:
            fprintf(stderr,
                    "ERROR: this packet should not be received! It will be ignored.\n");
            break;
    }
}

/*
 * Wait for any incoming data using select with 2seconds of timeout
 * Return a packet & status
 */
int wait_for_reply_using_select(int socket_fd, Packet **received_packet)
{
    //set of socket to be listened setup
    fd_set ready_set;
    FD_ZERO(&ready_set);
    FD_SET(socket_fd, &ready_set);

    //Timeout set up
    struct timeval time_out;
    time_out.tv_sec = 2;
    time_out.tv_usec = 0;

    int num_ready;
    num_ready = select(socket_fd + 1, &ready_set, NULL, NULL, &time_out);
    if (num_ready == -1) {
        fprintf(stderr, "ERROR: internal error of select()\n");
        return ERR;
    }
        //the socket is ready
    else if (num_ready) {
        rio_t new_rio;
        Rio_readinitb(&new_rio, socket_fd);
        *received_packet = read_packet(&new_rio);
        if (*received_packet == NULL) {
            fprintf(stderr, "ERROR: No data received\n");
            return ERR;
        }
        return OK;
    }
        //time out reached
    else {
        fprintf(stderr,
                "ERROR: No connection attempt / no data received within 2 seconds\n");
        return ERR;
    }
}

/*
 * Return a unique id generated using time and processor time
 */
unsigned int get_unique_id(unsigned int portNum)
{
    return (((int) (time(NULL)) % 0xFFFF) << 16) | (portNum % 0xFFFF);
}
