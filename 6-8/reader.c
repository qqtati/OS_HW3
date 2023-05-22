#include <stdio.h>   
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define GET_BOOK 1
#define RETURN_BOOK 2

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int books_count;
    int client_socket;
    struct sockaddr_in server_addr;
    unsigned short server_port;
    int recv_msg_size;
    char *server_ip;

    if (argc != 4)
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port> <Books count>\n", argv[0]);
       exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);
    books_count = atoi(argv[3]);

    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");
    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sin_family      = AF_INET;        
    server_addr.sin_addr.s_addr = inet_addr(server_ip); 
    server_addr.sin_port        = htons(server_port);
    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("connect() failed");

    int buffer[2];
    int decidedBook = 0;
    int gotBook = 0;
    int book_index = -1;
    for (;;) {
        if (!decidedBook) {
            book_index = rand() % books_count;
            decidedBook = 1;
            printf("Trying to get book #%d\n", book_index);
        } 
        if (gotBook) {
            buffer[0] = RETURN_BOOK;
            buffer[1] = book_index;
        }
        else {
            buffer[0] = GET_BOOK;
            buffer[1] = book_index;
        }
        send(client_socket, buffer, sizeof(buffer), 0);
        recv(client_socket, buffer, sizeof(buffer), 0);
        if (buffer[0] == 1) {
            printf("Got the book #%d\n", book_index);
            gotBook = 1;
            sleep(2 + rand() % 3);
            printf("Returning book #%d\n", book_index);
        } else if (buffer[0] == 2) {
            printf("Waiting for the book #%d\n", book_index);
        } else {
            gotBook = 0;
            decidedBook = 0;
        }
        sleep(1);
    }
    close(client_socket);
    return 0;
}
