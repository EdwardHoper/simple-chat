#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "utils.h"

void *handle_receving_msg(void *arg)
{
    int sock = (int)arg;
    char msg[256];

    while (1)
    {
        if (recv(sock, &msg, sizeof(msg), 0) == -1)
        {
            printf("Error: %s\n", strerror(errno));
        }

        puts(msg);
    }
}

int main()
{
    // Create socket
    int net_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (net_sock == -1)
    {
        printf("Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Specify an params for socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9002);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Create connection
    check_err(connect(net_sock, (struct sockaddr_in *)&server_addr, sizeof(server_addr)), "Connection error");

    // Create threads for receving messages
    pthread_t thread_1;
    pthread_create(&thread_1, NULL, *handle_receving_msg, (void *)net_sock);

    // Sending messages loop
    char msg[256];
    while (1)
    {
        printf("Input: ");
        fgets(&msg, sizeof(msg), stdin);

        if (send(net_sock, &msg, sizeof(msg), 0) == -1)
        {
            printf("Error: %s\n", strerror(errno));
        }
    }

    close(net_sock);

    return 0;
}