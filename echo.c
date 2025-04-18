#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

#define USAGE_ERR 1
#define PORT_INPUT_ERR 2

typedef struct {
    int client_fd;
    bool verbose;
} client_data_t;

int server_fd = -1;

void usage_msg(char* argv[]) {
    fprintf(stderr, "Usage: %s [-p <port>] [-v]\n", argv[0]);
    fprintf(stderr, "   -p: Use a user-specified port number\n");
    fprintf(stderr,
            "   -v: Print messages received from the client "
            "to the terminal\n");
}

// Read command line arguments and print an error if the inputs were invalid
// Sets values of port_num and verbose based on arguments given
int handle_args(int argc, char* argv[], int* port_num, bool* verbose) {
    bool p_arg_found = false;
    bool v_arg_found = false;

    // Detect if too many arguments
    if (argc > MAX_ARGS)
        return USAGE_ERR;

    // Read command line arguments for '-p' and '-v'
    for (int ix = 1; ix < argc; ix++) {
        if (strncmp(argv[ix], P_ARG_STR, ARG_LEN) == 0) {
            // Detect if the '-p' arg was already read, or if there was no
            // argument after '-p'
            if (p_arg_found || ix + 1 >= argc)
                return USAGE_ERR;

            // Print error if conversion to int failed or if the input is out of
            // range
            if (sscanf(argv[ix + 1], "%d", port_num) != 1 ||
                (*port_num <= 0 || *port_num > 65535))
                return PORT_INPUT_ERR;

            p_arg_found = true;

        } else if (strncmp(argv[ix], V_ARG_STR, ARG_LEN) == 0) {
            // Detect if '-v' arg was already read
            if (v_arg_found)
                return USAGE_ERR;
            *verbose    = true;
            v_arg_found = true;

            // If argument is not '-p' or '-v', and is not the port number given
            // after '-p', the argument must be invalid
        } else if (strncmp(argv[ix - 1], P_ARG_STR, ARG_LEN) != 0)
            return USAGE_ERR;
    }
    return 0;
}

// Initialize echo server using TCP, and print an error message if failed
int setup_server(struct sockaddr_in* servaddr, int port_num) {
    printf("Creating socket...\n");
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket() failed");
        return 1;
    }

    printf("Initializing server address...\n");
    memset(servaddr, 0, sizeof(*servaddr));
    servaddr->sin_family      = AF_INET;
    servaddr->sin_addr.s_addr = INADDR_ANY;
    servaddr->sin_port        = htons(port_num);
    printf("Server address initialized (port %d)\n", port_num);

    printf("Binding socket to address...\n");
    if (bind(server_fd, (struct sockaddr*)servaddr, sizeof(*servaddr)) == -1) {
        perror("bind() failed");
        return 1;
    }

    printf("Setting up server to listen for connections...\n");
    if (listen(server_fd, MAX_CONNECT_REQUESTS) == -1) {
        perror("listen() failed");
        return 1;
    }
    return 0;
}

// Read messages from the client and echo them back
void* read_client(void* input_ptr) {
    client_data_t* input = (client_data_t*)input_ptr;
    pthread_t thread_id  = pthread_self();
    char buffer[MAX_MSG_SIZE];

    printf("Client connected, handling in thread %lu\n",
           (unsigned long)thread_id);

    while (true) {
        memset(buffer, 0, MAX_MSG_SIZE);

        ssize_t bytes_read = read(input->client_fd, buffer, MAX_MSG_SIZE);
        if (bytes_read == -1) {
            perror("read() failed");
            continue;
        } else if (bytes_read == 0) {
            printf("Client disconnected\n");
            break;
        }

        buffer[bytes_read] = '\0';

        if (input->verbose) {
            printf("Thread %lu received: %s", (unsigned long)thread_id, buffer);
            // Print a newline if input did not have one
            if (buffer[bytes_read - 1] != '\n')
                printf("\n");
            printf("===================\n");
        }

        if (write(input->client_fd, buffer, bytes_read) == -1)
            perror("write() failed");
    }

    close(input->client_fd);
    printf("Client FD closed (thread: %lu)\n", (unsigned long)thread_id);
    free(input);
    return NULL;
}

void close_server() {
    close(server_fd);
    printf("\nServer FD closed\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    // Catch the SIGINT signal (Ctrl + C) to close the server properly
    signal(SIGINT, close_server);

    int port_num    = DEFAULT_PORT;
    bool verbose    = false;

    int input_state = handle_args(argc, argv, &port_num, &verbose);
    if (input_state == USAGE_ERR) {
        usage_msg(argv);
        return 1;
    } else if (input_state == PORT_INPUT_ERR) {
        fprintf(stderr, "Port number invalid or out of range\n");
        return 1;
    }

    struct sockaddr_in servaddr;

    if (setup_server(&servaddr, port_num) != 0)
        return 1;

    while (true) {

        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept() failed");
            continue;
        }

        client_data_t* data = malloc(sizeof(client_data_t));
        data->client_fd     = client_fd;
        data->verbose       = verbose;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, read_client, data) != 0) {
            perror("pthread_create() failed");
            return 1;
        }
        pthread_detach(thread_id);
        printf("Thread %lu created\n", thread_id);
    }
    return 0;
}