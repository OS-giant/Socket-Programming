#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10

struct q{
    char text[1024];
    int student_id;
    int status;
};

void sig_handler(int signo) {
    if (signo == SIGALRM) {
        printf("Time limit exceeded\n");
    }
}

void port_generator(int selected_index, int ta_socket_id, int student_socket_id) {
    srand(time(NULL));
    int port = rand() % 65536;

    char port_message[1024];
    sprintf(port_message, "The port for index %d is %d", selected_index, port);

    send(ta_socket_id, port_message, strlen(port_message), 0);
    send(student_socket_id, port_message, strlen(port_message), 0);
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket, activity, valread, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in address;
    char buffer[1024] = {0};
    fd_set readfds;
    int is_student;
    int is_ta;
    int client_num = 0;
    int chosen_q_index;
    struct q qes[MAX_CLIENTS];

    if(argc < 2){
        printf("port is not input\n");
        exit(EXIT_FAILURE);
    }
    // Create a socket for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow multiple connections on the same port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure for the server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the client socket array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    printf("Server listening on port %d\n", atoi(argv[1]));

    signal(SIGALRM, sig_handler);

    // Loop to handle incoming connections and client requests
    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the server socket to the socket set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add the client sockets to the socket set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR)) {
            perror("select error");
        }

        // Handle incoming connections
        socklen_t addrlen;
        addrlen = sizeof(address);
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("accept error");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add the new client socket to the array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Handle client requests
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // Close the socket and remove it from the array
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Broadcast the message to all other clients
                    buffer[valread] = '\0';
                    printf("Received message from client, socket fd is %d, message is: %s\n", sd, buffer);
                    if(is_student){
                        printf("is_student is active\n");
                        for(int i = 0 ; i < strlen(buffer) ; i++){
                            qes[client_num].text[i] = buffer[i];
                            qes[client_num].status = 1;
                            qes[client_num].student_id = sd;
                        }
                        printf("number : %d\n", qes[client_num].status);
                        printf("question text : %s", qes[client_num].text);
                        printf("fd : %d\n", qes[client_num].student_id);
                    } 
                    if(is_ta){
                        printf("TA is ready to answer\n");
                        printf("selected index is: %s\n", buffer);
                    }

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != 0 && client_sockets[j] != sd) {
                            send(client_sockets[j], buffer, strlen(buffer), 0);
                        }
                    }

                    if(strcmp(buffer, "ta\n") == 0 || strcmp(buffer, "TA\n") == 0){
                        is_ta = 1;
                        for(int i = 0 ; i < MAX_CLIENTS ; i++){  //send the list of not answered questions to the TA socket
                            if(qes[i].status == 1){
                                int ssd = send(sd, qes[i].text, strlen(qes[0].text), 0);
                                printf("%s", qes[i].text);
                            }
                        }
                        printf("A TA Entered\n");

                    } else if(strcmp(buffer, "student\n") == 0 || strcmp(buffer, "Student\n") == 0){
                        is_student = 1;
                        printf("Student's question : %s", buffer);
                    }
                }
            }
        }

        printf("client #%d", client_num);
        client_num++;
    }

    return 0;
}
