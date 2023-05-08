#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10

struct q{
    char text[1024];
    int student_id;
    int status;
};

void print_qes(struct q qes[]){
    //printf("it worked!");
    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        if(qes[i].status == 1){
            printf("%d. ", i);
            printf("%s", qes[i].text);
            printf("\n");
        }
    }
    printf("\n");
}

int main(int argc, char const *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    struct q qes[MAX_CLIENTS];
    int client_num = 0;

    if(argc < 2){
        printf("port is not input\n");
        return -1;
    }
    //initialization of the list
    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        qes[i].status = 0;
        qes[i].student_id = 0;
        strcpy(qes[i].text, "not_yet");
    }

    // Create a socket for the client
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Set up the address structure for the server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server on port %d\n", atoi(argv[1]));

    // Loop to send and receive messages
    int q_index;
    while(1) {
        printf("Enter your role number(1.TA/2.Student)");
        alarm(60);
        fgets(buffer, 1024, stdin);

        // Send the message to the server
        send(sock, buffer, strlen(buffer), 0);
        if(strcmp(buffer, "ta\n") == 0 || strcmp(buffer, "TA\n") == 0){
            print_qes(qes); 
            inputting : 
                printf("Choose an index to answer : \n");
                alarm(60);
                fgets(buffer, 1024, stdin);
                int selected_index = atoi(buffer);
            if(selected_index > -1 && selected_index < MAX_CLIENTS){
                send(sock, buffer, strlen(buffer), 0);
            } else {
                printf("this index does not belong to the list.\n");
                goto inputting;
            }
             
        } else if(strcmp(buffer, "student\n") == 0 || strcmp(buffer, "Student\n") == 0){
            printf("Ask a question : \n$ ");
            alarm(60);
            fgets(buffer, 1024, stdin);
            send(sock, buffer, strlen(buffer), 0);
            
            for(int i = 0 ; i < strlen(buffer) ; i++){
                qes[client_num].text[i] = buffer[i];
                qes[client_num].status = 1;
                qes[client_num].student_id = sock;
            }

            printf("your question featues was recorded on server : \n");
            printf("number : %d, ", qes[client_num].status);
            printf("fd : %d, ", qes[client_num].student_id);
            printf("question text : %s", qes[client_num].text);
            printf("and is waiting for a TA who can answers it. Keep patient!\n");
            client_num++;
        } 

        // Receive the response from the server
        valread = read(sock, buffer, 1024);
        //printf("Server response: %s", buffer);
        
    }

    return 0;
}