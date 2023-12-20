#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>

// Flag to control the main loop
volatile sig_atomic_t keep_running = 1;

// Signal handler function
void handle_signal(int signo) {
    // The signal handler only sets the flag to terminate the main loop
    printf("Handling signal\n");
    keep_running = 0;
}

int main(int argc, char *argv[]) {
    // Check for the correct number of command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Declare and initialize variables
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    // Create a socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }

    // Register signal handler for SIGHUP
    signal(SIGHUP, handle_signal);

    printf("Server is running on port %d\n", port);

    // Set up pselect timeout and signal masking
    struct timespec timeout = {5, 0}; // 5 seconds
    sigset_t blockedMask, origMask;

    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);

    // Block SIGHUP
    if (sigprocmask(SIG_BLOCK, &blockedMask, &origMask) == -1) {
        perror("sigprocmask error");
        return 1;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);
    int max_fd = server_socket;

    while (keep_running) {
        // Copy the file descriptor set as pselect modifies it
        fd_set temp_fds = readfds;

        // Wait for activity on sockets using pselect
        int ready_fds;
        do {
            ready_fds = pselect(max_fd + 1, &temp_fds, NULL, NULL, &timeout, &origMask);
        } while (ready_fds == -1 && errno == EINTR);

        if (ready_fds == -1) {
            perror("pselect error");
            break;
        } else if (ready_fds == 0) {
            printf("No activity on sockets\n");
        } else {
            // Check for data on client sockets
            for (int i = server_socket + 1; i <= max_fd; i++) {
                if (FD_ISSET(i, &temp_fds)) {
                    char buffer[1024];
                    ssize_t bytes_read = recv(i, buffer, sizeof(buffer), 0);

                    if (bytes_read == -1) {
                        perror("Recv failed");
                    } else if (bytes_read == 0) {
                        // Connection closed by the client
                        printf("Connection closed by client\n");
                        close(i);
                        FD_CLR(i, &readfds);
                    } else {
                        printf("%s", buffer);
                        // Process data if needed
                    }
                }
            }

            // Check for incoming connections
            if (FD_ISSET(server_socket, &temp_fds)) {
                if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                    perror("Accept failed");
                } else {
                    // Log the new connection
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                    printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

                    // Add the client socket to the set
                    FD_SET(client_socket, &readfds);

                    // Update max_fd if necessary
                    if (client_socket > max_fd) {
                        max_fd = client_socket;
                    }
                }
            }
        }
    }

    // Close all sockets
    for (int i = server_socket; i <= max_fd; i++) {
        if (FD_ISSET(i, &readfds)) {
            close(i);
        }
    }

    printf("Server is shutting down\n");
    return 0;
}

