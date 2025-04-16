#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define MAX_MSG_SIZE 256
#define MAX_CONNECT_REQUESTS 5

#define PORT_ARG 1
#define PORT_NUM_ARG 2
#define PRINT_ARG 3

// TODO: implement -p
// TODO: implement -v argument

int main(int argc, char *argv[]) {
    char buffer[MAX_MSG_SIZE];
    int server_fd;
    int client_fd;
    struct sockaddr_in servaddr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    // todo: change this to use the port inputted by the user if its provided
    servaddr.sin_port = htons(DEFAULT_PORT);

    if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_fd, MAX_CONNECT_REQUESTS) == -1) {
        perror("Listen failed");
        return 1;
    }

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("Accept failed");
        return 1;
    }

    // Read strings sent by the client and echo them back until the connection
    // is terminated
    while (1) {
        memset(buffer, 0, MAX_MSG_SIZE);

        ssize_t bytes_read = read(client_fd, buffer, MAX_MSG_SIZE);

        if (bytes_read == -1) {
            perror("Read failed");
            continue;
        } else if (bytes_read == 0) {
            printf("Client disconnected\n");
            break;
        }
        printf("%s\n", buffer);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}