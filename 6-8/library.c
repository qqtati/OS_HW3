#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXPENDING 5

pthread_mutex_t mutex;
int *books;
int books_count;
char message[32];

typedef struct thread_args {
    int socket;
} thread_args;

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void *handleClient(void *args) {
    int socket;
    pthread_detach(pthread_self());
    socket = ((thread_args*)args)->socket;
    free(args);
    int buffer[2];
    for (;;) {
        recv(socket, buffer, sizeof(buffer), 0);
        pthread_mutex_lock(&mutex);
        if (buffer[0] == 1) {
            if (books[buffer[1]] == 0) {
                printf("Giving book %d to reader\n", buffer[1]);
                snprintf(message, sizeof(message), "Giving book %d to reader\n", buffer[1]);
                books[buffer[1]] = 1;
                buffer[0] = 1;
            } else {
                snprintf(message, sizeof(message), "Waiting for book #%d\n", buffer[1]);
                buffer[0] = 2;
            }
        } else {
            printf("Got returned book #%d\n", buffer[1]);
            snprintf(message, sizeof(message), "Got returned book #%d\n", buffer[1]);
            books[buffer[1]] = 0;
            buffer[0] = 3;
        }
        pthread_mutex_unlock(&mutex);
        send(socket, buffer, sizeof(buffer), 0);
    }
    close(socket);
}

void *handleObserver(void *args) {
    int socket;
    pthread_detach(pthread_self());
    socket = ((thread_args*)args)->socket;
    free(args);
    printf("Observer connected\n");
    int buffer[1];
    buffer[0] = 0;
    for (;;) {
        send(socket, message, sizeof(message), 0);
        recv(socket, buffer, sizeof(buffer), 0);
        if (buffer[0] == -1) break;
        sleep(1);
    }
    printf("Observer disconnected\n");
    close(socket);
}

void *clientThread(void *args) {
    int server_socket;
    int client_socket;
    int client_length;
    pthread_t threadId;
    struct sockaddr_in client_addr;
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    free(args);
    listen(server_socket, MAXPENDING);
    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));

        thread_args *args = (thread_args*) malloc(sizeof(thread_args));
        args->socket = client_socket;
        if (pthread_create(&threadId, NULL, handleClient, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
}

int createTCPSocket(unsigned short server_port) {
    int server_socket;
    struct sockaddr_in server_addr;

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;              
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(server_port);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("bind() failed");
    printf("Open socket on %s:%d\n", inet_ntoa(server_addr.sin_addr), server_port);
    return server_socket;
}

int main(int argc, char *argv[])
{
    unsigned short server_port;
    unsigned short server_extra_port;
    int server_socket;
    int server_extra_socket;
    pthread_t threadId;

    struct sockaddr_in client_addr;
    int client_socket;
    int client_length;

    pthread_mutex_init(&mutex, NULL);
    if (argc != 4)
    {
        fprintf(stderr, "Usage:  %s <Port for clients> <Port for observer> <Books count>\n", argv[0]);
        exit(1);
    }

    server_port = atoi(argv[1]);
    server_extra_port = atoi(argv[2]);
    books_count = atoi(argv[3]);
    books = (int*) malloc(sizeof(int) * books_count);
    server_socket = createTCPSocket(server_port);
    server_extra_socket = createTCPSocket(server_extra_port);


    thread_args *args = (thread_args*) malloc(sizeof(thread_args));
    args->socket = server_socket;
    if (pthread_create(&threadId, NULL, clientThread, (void*) args) != 0) DieWithError("pthread_create() failed");

    listen(server_extra_socket, MAXPENDING);
    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_extra_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));

        thread_args *args1 = (thread_args*) malloc(sizeof(thread_args));
        args1->socket = client_socket;
        if (pthread_create(&threadId, NULL, handleObserver, (void*) args1) != 0) DieWithError("pthread_create() failed");
    }
    free(books);
    pthread_mutex_destroy(&mutex);
    return 0;
}