#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_helper.h"
#include "tasks.h"
#include "data_structure.h"
#include "packet_helper.h"
#include "get_options.h"

int main(int argc, char **argv)
{
    //by default neighbour super is not known
    neighbourSuperIsUpdated = 0;

    //Options
    if (argc < 3) {
        fprintf(stderr,
                "#Usage : ./super -p XXX [--s_ip=XXX --s_port=XXX]\n");
        return ERR;
    }

    //Listening port number & super info obtained from options
    int listeningPortNumber = 11111; //default port number
    char *neighbourSuperNodeIp;
    neighbourSuper.portNum = 11115; //default port number
    //Read the new informations from the options
    if (getOptions(argc, argv, (unsigned int *) &listeningPortNumber,
                   &neighbourSuperNodeIp, &(neighbourSuper.portNum)) == ERR) {
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
     * STEP 2 : CONNECT to its neighbour Super Node & DO Handshake
     * ------------------------------------------------------------------------
     */
    if (argc > 3) {
        strncpy(neighbourSuper.ipAddr, neighbourSuperNodeIp,
                strlen(neighbourSuperNodeIp));
        free(neighbourSuperNodeIp);
        neighbourSuperIsUpdated = 1;

        //new tcp connection
        int neighbour_supernode_fd = client_connecting(neighbourSuper.ipAddr,
                                                       neighbourSuper.portNum);

        //handshake initialise
        if (send_hello_packet(SUPER2SUPER_HELLO, neighbour_supernode_fd, app_id,
                              (unsigned int) (listeningPortNumber)) == ERR) {
            fprintf(stderr, "ERROR: failed to send Super2Super Hello\n");
            return ERR;
        }
    }

    /*
     * ------------------------------------------------------------------------
     * STEP 3 : LISTENING to the server socket for any attemp connection from
     *          super or child
     * ------------------------------------------------------------------------
     */
    //Timeout set up
    struct timeval time_out;
    time_out.tv_sec = 2;
    time_out.tv_usec = 0;
    while (1) {
        new_pool.ready_read_set = new_pool.active_read_set;
        new_pool.num_ready = select(new_pool.max_active_read_fd + 1,
                                    &(new_pool.ready_read_set),
                                    NULL, NULL, &time_out);

        //if we detect input from listening socket
        if (FD_ISSET(server_fd, &(new_pool.ready_read_set))) {
            //STEP 1 : Accepting a new request from the client
            client_fd = accept(server_fd,
                               (struct sockaddr *) (&clientAddress),
                               &addLength);
            if (client_fd < 0) {
                continue;
            }
            //add a new client socket
            add_client(client_fd, clientAddress, &new_pool);
        }
        //create a worker thread for added clients
        check_clients_super(&new_pool);

    }
    close(server_fd);

    return OK;
}
