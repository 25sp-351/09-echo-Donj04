#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define MAX_MSG_SIZE 1024
#define MAX_CONNECT_REQUESTS 5

#define P_ARG_STR "-p"
#define V_ARG_STR "-v"
#define ARG_LEN 3
#define MAX_ARGS 4  // including argv[0]

void usage_msg(char* argv[]) {
    fprintf(stderr, "Usage: %s [-p <port>] [-v]\n", argv[0]);
    fprintf(stderr, "   -p: Use a user-specified port number\n");
    fprintf(stderr,
            "   -v: Print messages received from the client "
            "to the terminal\n");
}

// Initialize echo server using TCP, and print an error message if failed
int setup(int* server_fd, int* client_fd, struct sockaddr_in* servaddr,
          int port_num) {

    printf("Creating socket...\n");
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    printf("Initializing server address...\n");
    memset(servaddr, 0, sizeof(*servaddr));
    servaddr->sin_family      = AF_INET;
    servaddr->sin_addr.s_addr = INADDR_ANY;
    servaddr->sin_port        = htons(port_num);
    printf("Server address initialized (port %d)\n", port_num);

    printf("Binding socket to address...\n");
    if (bind(*server_fd, (struct sockaddr*)servaddr, sizeof(*servaddr)) == -1) {
        perror("bind() failed");
        return 1;
    }

    printf("Setting up server to listen for connections...\n");
    if (listen(*server_fd, MAX_CONNECT_REQUESTS) == -1) {
        perror("listen() failed");
        return 1;
    }

    printf("Waiting for client to connect...\n");
    *client_fd = accept(*server_fd, NULL, NULL);
    if (*client_fd == -1) {
        perror("accept() failed");
        close(*server_fd);
        return 1;
    }

    return 0;
}

// Read messages from the client and echo them back
void read_client(int client_fd, bool verbose) {
    char buffer[MAX_MSG_SIZE];

    printf("Client connected\n");

    while (true) {
        memset(buffer, 0, MAX_MSG_SIZE);

        if (verbose)
            printf("===================\n");

        ssize_t bytes_read = read(client_fd, buffer, MAX_MSG_SIZE);
        if (bytes_read == -1) {
            perror("read() failed");
            continue;
        } else if (bytes_read == 0) {
            printf("Client disconnected\n");
            break;
        }

        buffer[bytes_read] = '\0';

        if (verbose)
            printf("Received: %s\n", buffer);

        if (write(client_fd, buffer, bytes_read) == -1)
            perror("write() failed");
    }
}

int main(int argc, char* argv[]) {
    int port_num     = DEFAULT_PORT;
    bool verbose     = false;

    bool p_arg_found = false;
    bool v_arg_found = false;

    // Detect if too many arguments
    if (argc > MAX_ARGS) {
        usage_msg(argv);
        return 1;
    }
    // Read command line arguments for '-p' and '-v'
    for (int ix = 1; ix < argc; ix++) {
        if (strncmp(argv[ix], P_ARG_STR, ARG_LEN) == 0) {
            // Detect if the '-p' arg was already read, or if there was no
            // argument after '-p'
            if (p_arg_found || ix + 1 >= argc) {
                usage_msg(argv);
                return 1;
            }
            char* arg = argv[ix + 1];
            // Print error if conversion to int failed or if the input is out of
            // range
            if (sscanf(arg, "%d", &port_num) != 1 ||
                (port_num <= 0 || port_num > 65535)) {
                fprintf(stderr, "Port number invalid or out of range: %s\n",
                        arg);
                return 1;
            }
            p_arg_found = true;

        } else if (strncmp(argv[ix], V_ARG_STR, ARG_LEN) == 0) {
            // Detect if '-v' arg was already read
            if (v_arg_found) {
                usage_msg(argv);
                return 1;
            }
            verbose     = true;
            v_arg_found = true;

        } else {
            printf("%s\n", argv[ix]);
        }
    }

    int server_fd;
    int client_fd;
    struct sockaddr_in servaddr;

    if (setup(&server_fd, &client_fd, &servaddr, port_num) != 0)
        return 1;

    read_client(client_fd, verbose);

    close(client_fd);
    printf("Client FD closed\n");
    close(server_fd);
    printf("Server FD closed\n");
    return 0;
}