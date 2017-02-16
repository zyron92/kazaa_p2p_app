#ifndef __TASKS_H__
#define __TASKS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "socket_helper.h"
#include "packet_handler.h"
#include "packet_helper.h"
#include "data_structure.h"
#include "file_handler.h"
#include "msg_type.h"
#include "hash.h"
#include "hash2.h"

/*
 * Initialisation of socket pool
 */
void init_pool(int listen_fd, Pool *p);

/*
 * Add a new client to the Pool
 */
void add_client(int connfd, struct sockaddr_in clientAddress, Pool *p);

/*
 * Check added clients of child and do a task for each of them is they are active
 */
void check_clients_child(Pool *p);

/*
 * Check added clients of super and do a task for each of them is they are active
 */
void check_clients_super(Pool *p);

/*
 * Download the requested file after got the information from the supernode
 */
void do_download(const char *buffer, int sizeBytes);

/*
 * Do a task for super according to received packet from the listening socket
 */
void do_task_super(Packet *received_packet, int connfd,
                   struct sockaddr_in clientAddress);

/*
 * Do a task for child according to received packet from the listening socket
 * Only for the requesting and
 */
void do_task_child(Packet *received_packet, int connfd);

/*
 * Wait for any incoming data using select with 2seconds of timeout
 * Return a packet & status
 */
int wait_for_reply_using_select(int socket_fd, Packet **received_packet);

/*
 * Return a unique id generated using time and processor time
 */
unsigned int get_unique_id();

#endif
