#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#define MAX_CONN 3
char *ip_address = "127.0.0.1";
int port_number = 25555;
void printUsage(char *name) {
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h] \n", name);
    printf("-f question_file    Default to \"question.txt\"; \n");
    printf("-i IP_address       Default to \"127.0.0.1\"; \n");
    printf("-p port_address     Default to 25555; \n");
    printf("-h                  Display this help info.\n");

}
void parse_connect(int argc, char** argv, int* sever_fd) {
  
  char *question_file = "questions.txt";
        
        int opt;
        while((opt = getopt(argc, argv, "f:i:p:h")) != -1) {
            switch(opt) {
                case 'f':
                    question_file = optarg;
                    break;
                case 'i':
                    ip_address = optarg;
                    break;
                case 'p':
                    port_number = atoi(optarg);
                    break;
                case 'h':
                    printUsage(argv[0]);  
                case '?':
                    printf("Error: Unknown option '%c' received. \n", optopt);
            }
        }
}
// make the set contain stdin and the server socket
      // select on those two
      // whenever select returns
      // then check if the server socket is checked (question)
      // otherwise stdin is set, read the charecter from stdinpt and send

int main(int argc, char* argv[]){
    int    client_fd ;
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);
    parse_connect(argc, argv, &client_fd );
    

    /*STEP 1:
    Create a socket to talk to the server;
    */
    client_fd  = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    
    /*STEP 2:
    Try to connect to the server.
    */
    if (connect(client_fd , (struct sockaddr *) &server_addr, addr_size) == -1) {
      perror("connect");
      exit(1);
    }
    char  buffer[2048];
    while(1) {
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(STDIN_FILENO, &read_fds);
      FD_SET(client_fd, &read_fds);
        // monitor file descriptor set
      
      if (select(client_fd+1, &read_fds, NULL, NULL, NULL) == -1) {
        perror("select");
        exit(1);
      }
      if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            // Read user input from stdin
            char answer[10];
           if (scanf("%s", answer) != 1) {
              printf("scanf failed to read an integer\n");
          }
            // Send the message to the server
            if (send(client_fd, answer, strlen(answer), 0) == -1) {
                perror("Send failed");
                exit(EXIT_FAILURE);
            }
        }

        // Check if there's data from the server
        if (FD_ISSET(client_fd, &read_fds)) {
            // Receive response from the server
            ssize_t bytes_received = recv(client_fd, buffer, 1024, 0);
            if (bytes_received == -1) {
                perror("Receive failed");
                exit(EXIT_FAILURE);
            } else if (bytes_received == 0) {
                printf("Server closed the connection\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Game Master Says: %s\n", buffer);
        }
    }

    // printf("Welcome to 392 trivia!!!!! \n");
    // printf("Please enter your name!\n");

    // printf("[Client]: "); fflush(stdout);
    // scanf("%s", buffer);


    close(client_fd );

  return 0;
}