#include "socket_helper.h"

/*
 * Creating a server socket, binding it to 0.0.0.0:portNumber and preparing it
 * Binding to all interfaces with the port number specified
 * Child : for listening file download request
 * Super : for listening file info sharing & hello
 */
int server_listening(int portNumber)
{    //STEP 1 : Creating a socket descriptor for the server
    int resultSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (resultSocket < 0) {
        fprintf(stderr, "ERROR: failed to establish a server socket \
[socket()]\n");
        return ERR;
    }

    //Removing the binding error for a faster debugging &
    //Allowing reuse of adresses
    int optValue = 1;
    if (setsockopt(resultSocket, SOL_SOCKET, SO_REUSEADDR,
                   (const void *) (&optValue), sizeof(int)) < 0) {
        fprintf(stderr, "ERROR: failed to establish a server socket \
[setsockopt()]\n");
        return ERR;
    }

    //Setting the socket to non-blocking
    if ((optValue = fcntl(resultSocket, F_GETFL)) < 0) {
        fprintf(stderr, "ERROR: failed to establish a server socket \
[fcntl()] -getting current options\n");
    }
    optValue = (optValue | O_NONBLOCK);
    if (fcntl(resultSocket, F_SETFL, optValue)) {
        fprintf(stderr, "ERROR: failed to establish a server socket \
[fcntl()] -modifying and applying\n");
    }

    //STEP 2 : Binding process
    //we use calloc to initialize the structure with zeros
    struct sockaddr_in *serverAddress = calloc(1,
                                               sizeof(struct sockaddr_in));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(portNumber);
    inet_pton(AF_INET, "0.0.0.0", &(serverAddress->sin_addr));
    if (bind(resultSocket, (struct sockaddr *) (serverAddress),
             sizeof(struct sockaddr_in)) < 0) {
        free(serverAddress);
        fprintf(stderr, "ERROR: failed to establish a server socket \
[bind()]\n");
        return ERR;
    }

    //STEP 3 : Making the socket ready to accept incomming connection
    if (listen(resultSocket, 1000) < 0) { //1000 clients accepted in the queue
        free(serverAddress);
        fprintf(stderr, "ERROR: failed to establish a server socket \
[listen()]\n");
        return ERR;
    }

    free(serverAddress);
    return resultSocket;
}

/*
 * Creating a client socket, and connecting it to ipAddress:portNum
 */
int client_connecting(const char *ipAddress, int portNum)
{
    //STEP 1 : Creating a socket descriptor for the client
    int resultSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (resultSocket < 0) {
        fprintf(stderr, "ERROR: failed to establish a client socket \
[socket()]\n");
        return ERR;
    }

    //STEP 2 : Connecting process to the server
    //we use calloc to initialize the structure with zeros
    struct sockaddr_in *serverAddress = calloc(1,
                                               sizeof(struct sockaddr_in));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(portNum);
    inet_pton(AF_INET, ipAddress, &(serverAddress->sin_addr));
    if (connect(resultSocket, (struct sockaddr *) (serverAddress),
                sizeof(struct sockaddr_in)) < 0) {
        free(serverAddress);
        fprintf(stderr, "ERROR: failed to establish a client socket \
[bind()]\n");
        return ERR;
    }

    free(serverAddress);
    return resultSocket;
}

/*
 * Getting client socket information
 */
int get_connected_client_info(struct sockaddr_in *client_socket_info,
                              char **ipAddress, int *portNum)
{
    if (client_socket_info == NULL) {
        fprintf(stderr, "ERROR: failed to get client socket info\n");
        return ERR;
    }

    *ipAddress = inet_ntoa(client_socket_info->sin_addr);
    *portNum = ntohs(client_socket_info->sin_port);

    return OK;
}

/*
 * Return the ip address after the resolution of domain name if it is a domain
 * name, or else return itself if it is already an ip address
 *
 * Free is required
 */
char *resolve_domain_name(const char *domainName)
{
    if (domainName == NULL) {
        return NULL;
    }

    char *ipAddress;

    struct in_addr address;
    //If it is an ip address, we just send a copy of itself
    if (inet_aton(domainName, &address) != 0) {
        ipAddress = calloc(strlen(domainName) + 1, sizeof(char));
        strncpy(ipAddress, domainName, strlen(domainName));
    }
        //Or if it is a domain name, we try to resolve it
    else {
        //Resolution of domain name
        struct hostent *hostInfo;
        hostInfo = gethostbyname(domainName);

        if (hostInfo == NULL) {
            fprintf(stderr, "ERROR1: resolution of domain name \
failed!\n");
            return NULL;
        }

        if (hostInfo->h_addr_list == NULL) {
            fprintf(stderr, "ERROR2: resolution of domain name \
failed!\n");
            return NULL;
        }
        //we take the first answer of ip address answer corresponding
        //to the domain name and send it as a string
        address.s_addr = ((struct in_addr *)
                (hostInfo->h_addr_list[0]))->s_addr;
        char *ipAddressTemp = inet_ntoa(address);
        int ipAddressLength = strlen(ipAddressTemp);
        ipAddress = calloc(ipAddressLength + 1, sizeof(char));
        strncpy(ipAddress, ipAddressTemp, ipAddressLength);
    }

    return ipAddress;
}
