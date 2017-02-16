#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_helper.h"
#include "tasks.h"
#include "packet_helper.h"
#include "msg_type.h"
#include "file_handler.h"
#include "data_structure.h"
#include "get_options.h"

int main(int argc, char **argv)
{
    //Options
    if (argc != 5) {
        fprintf(stderr,
                "#Usage : ./child -p XXX --s_ip=XXX --s_port=XXX\n");
        return ERR;
    }

    //Listening port number & super info obtained from options
    int listeningPortNumber = 11113; //default port number
    superNodePortNumber = 11112; //default port number
    //Read the new informations from the options
    if (getOptions(argc, argv, (unsigned int *) (&listeningPortNumber),
                   &superNodeIp, &superNodePortNumber) == ERR) {
        return ERR;
    }

    /*
     * ------------------------------------------------------------------------
     * STEP 1 : Preparing the super node listening server, the connection pool
     *          and client information
     * ------------------------------------------------------------------------
     */
    //get unique id
    app_id = get_unique_id(listeningPortNumber);
    int server_fd = server_listening(listeningPortNumber);
    if (server_fd == ERR) {
        fprintf(stderr, "ERROR: failed to establish a server socket\n");
        return ERR;
    }
    //pool
    Pool new_pool;
    init_pool(server_fd, &new_pool);
    //client info
    int client_fd;
    struct sockaddr_in clientAddress;
    socklen_t addLength = (socklen_t)(sizeof(struct sockaddr_in));

    /*
     * ------------------------------------------------------------------------
     * STEP 2 : CONNECT to its Super Node & DO Handshake
     * ------------------------------------------------------------------------
     */
    //supernode fd
    int supernode_fd;
    //new tcp connection
    supernode_fd = client_connecting(superNodeIp, superNodePortNumber);
    //handshake initialise
    if (send_hello_packet(CHILD_HELLO, supernode_fd, app_id,
                                (unsigned int) (listeningPortNumber)) == ERR) {
        fprintf(stderr, "ERROR: failed to send Child Hello\n");
        return ERR;
    }
    //handshake finish by listening to any incoming packet
    Packet *reply_hello_packet;
    if (wait_for_reply_using_select(supernode_fd, &reply_hello_packet) == ERR) {
        fprintf(stderr, "ERROR: failed to receive any packet\n");
        return ERR;
    }
    //verify received packet for handshake
    if (receive_generic_packet(reply_hello_packet, SUPER_HELLO, 1) == ERR) {
        fprintf(stderr, "ERROR: failed to receive Super Hello\n");
        return ERR;
    }

    /*
     * ------------------------------------------------------------------------
     * STEP 3 : SCAN LOCAL DIRECTORY & Share the list of file with super node
     * ------------------------------------------------------------------------
     */
    unsigned int numFiles;
    //scan
    HashTableElem2 *file_list = scan_folder("data", (int *) (&numFiles));
    if (file_list == NULL) {
        fprintf(stderr,
                "ERROR: No file detected in the directory. Skipping the file info share to Super Node.\n");
    } else {
        //new tcp connection
        supernode_fd = client_connecting(superNodeIp, superNodePortNumber);
        //send files info
        if (send_file_info_packet(FILE_INFO, supernode_fd, app_id, file_list,
                                  numFiles) == ERR) {
            fprintf(stderr, "ERROR: failed to send file info packet\n");
            return ERR;
        }

        //receive reply file info by listening to any incoming packet
        Packet *reply_file_info_packet;
        if (wait_for_reply_using_select(supernode_fd,
                                        &reply_file_info_packet) ==
            ERR) {
            fprintf(stderr, "ERROR: failed to receive any packet\n");
            return ERR;
        }

        //verify received packet for file info sharing
        HashTableElem2 *all_files;
        unsigned int num_files;
        if (receive_file_info_recv_packet(reply_file_info_packet, &num_files,
                                          &all_files, FILE_INFO_OK, 0) == ERR) {
            if (receive_generic_packet(reply_file_info_packet, FILE_INFO_ERR,
                                       0) ==
                ERR) {
                fprintf(stderr,
                        "ERROR: failed to receive a correct File Info Reply\n");
                return ERR;
            } else {
                fprintf(stderr,
                        "ERROR: File Info sharing failed, so this child stops here.\n");
                return ERR;
            }
        }
    }

    /*
     * ------------------------------------------------------------------------
     * STEP 4 : LISTENING to the server socket and stdin for downloading
     *          or requesting files
     * ------------------------------------------------------------------------
     */
    //Timeout set up
    struct timeval time_out;
    time_out.tv_sec = 2;
    time_out.tv_usec = 0;
    //Active fd for stdin
    FD_SET(0, &(new_pool.active_read_set));
    while (1) {
        new_pool.ready_read_set = new_pool.active_read_set;
        new_pool.num_ready = select(new_pool.max_active_read_fd + 1,
                                    &(new_pool.ready_read_set),
                                    NULL, NULL, &time_out);
        //if we detect input from listening socket
        if (FD_ISSET(server_fd, &(new_pool.ready_read_set))) {
            //Accepting a new request from the client
            client_fd = accept(server_fd,
                               (struct sockaddr *) (&clientAddress),
                               &addLength);
            if (client_fd < 0) {
                continue;
            }
            //add a new client socket
            add_client(client_fd, clientAddress, &new_pool);
        }

        //if we detect input from STDIN
        if (FD_ISSET(0, &(new_pool.ready_read_set))) {
            new_pool.num_ready--;
            //init the buffer read for stdin
            rio_t rio;
            rio_readinitb(&rio, 0);
            char *buffer_stdin = calloc(1000, sizeof(char));
            int readBytesStdin = rio_readlineb(&rio, buffer_stdin, 1000);

            //start interpreting local request
            do_download(buffer_stdin, readBytesStdin);
            fflush(stderr);

            //if detect 'q' from stdin, we quit the program
            if (buffer_stdin[0] == 'q') {
                break;
            }
        }

        //Check clients if there is task to do
        check_clients_child(&new_pool);

    }
    close(server_fd);

    return OK;
}
