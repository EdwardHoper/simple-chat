#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "utils.h"

#define MAX_CLIENTS 10

struct connection_thread_args
{
    int server_sock;
};

typedef struct
{
    struct sockaddr_in address;
    int sock_fd;
    int uid;
    char name[64];
    char *ip;
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void spread_msg(client_t *sender_client, char *msg)
{
    /*
        Spread msg from one client to all of them
    */
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->uid != sender_client->uid)
        {
            check_err(send(clients[i]->sock_fd, msg, sizeof(msg), 0), "Sending error");
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void add_client(client_t *client)
{
    /*
        Add new client to the stack of clients
        if there is a space for it
    */
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i])
        {
            clients[i] = client;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid)
{
    /*
        Remove client from the stack by its uid
    */
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->uid == uid)
        {
            clients[i] = NULL;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg)
{
    client_t *client = (client_t *)arg;

    // Print client ip
    printf("Connected: %s\n", client->ip);

    // Listening
    while (1)
    {
        // Recive data
        char data[256];
        recv(client->sock_fd, &data, sizeof(data), 0);
        puts(data);

        // Resend recived data TEST
        spread_msg(client, data);
        // check_err(send(client->sock_fd, &data, sizeof(data), 0), "Send error");
    }

    // Close socket and remove client
    close(client->sock_fd);
    remove_client(client->uid);
    free(client);
    pthread_detach(pthread_self());

    return NULL;
}

int main()
{
    const char message[] = "Witaj na serwerze!!!";

    // Create socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int option = 1;
    check_err(
        setsockopt(server_sock, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)),
        "setsockopt error");

    // Define server params
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9002);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind connection and listen
    check_err(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)), "bind error");
    check_err(listen(server_sock, 100), "listen error");

    pthread_t t_id;

    while (1)
    {
        // Accept new client
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);
        check_err(client_sock, "Accept error");

        // Set client's settings
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = client_addr;
        client->sock_fd = client_sock;
        client->ip = inet_ntoa(client_addr.sin_addr);
        srand(time(NULL));
        client->uid = rand(); // Get random uid

        // Add client to queue
        add_client(client);

        // Create new thread for each user
        pthread_create(&t_id, NULL, *handle_client, (void *)client);

        // pthread_join(t_id, NULL);

        sleep(1);
    }

    return 0;
}