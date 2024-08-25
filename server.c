#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#define MAX_CONN 3
// struct for Entry contains questions, possible answers, and the correct answer
struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};
// contains the players fd, score, and name
struct Player {
    int fd;
    int score;
    char name[128];
};
struct Entry2 {
    char first[1024];
    };
// remove the white spaces from the questions, allows me to compare and find the correct answer index
void trim(char *str) {
    // Trim leading whitespace
    char *start = str;
    while (isspace(*start)) {
        start++;
    }

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) {
        end--;
    }

    // Move trimmed string to the beginning of the buffer
    memmove(str, start, end - start + 1);

    // Null-terminate the trimmed string
    str[end - start + 1] = '\0';
}
// this parses the questions
int read_questions(struct Entry* arr, char* filename) {
    FILE* fp = fopen(filename, "r"); // ope the file
    if(fp == NULL) {
        perror("fopen");
        exit(1);// error checking
    } 
    char currentLine[1024]; //buffer
    int i =0; // counter for index
    int num_questions = 0; // to count how many questions there ar ein the file
    while((fgets(currentLine, sizeof(currentLine), fp)) != NULL) { // we use fgets to get the line
        i++;
        if(i % 4 == 1) { // every four lines there will be question
            strcpy(arr[(i-1) /4].prompt, currentLine); // get and store the string into the struct
           //printf("%s", arr[(i-1) /4].prompt);
           num_questions++; // increment the question count
        }
        if(i % 4 == 2) { // this gets the options
            char* token = strtok(currentLine, " "); // seperate the options through a whtie space
            
            for(int j = 0; j < 3; j++) { // cycle through all three options
               
                strcpy(arr[(i-1)/4].options[j], token); // copy the option and index into the struct
                token = strtok(NULL, " "); 
                
            }    
        }
         if(i % 4 == 3) { // now we have to compare the options to teh string to get the correct index
            char answer[50];
            strcpy(answer, currentLine); // copy the correct answer
            trim(answer); // remove the extra whitespace so i can actually check the answer
            trim(arr[(i-1)/4].options[2]); // also get rid of the white space
            if (strcmp(answer, arr[(i - 1) / 4].options[0]) == 0) { // now we check each options and if it is we set the index to 1
                arr[(i - 1) / 4].answer_idx = 1; 
            } else if (strcmp(answer, arr[(i - 1) / 4].options[1]) == 0) {
                arr[(i - 1) / 4].answer_idx = 2; 
            }
            else if (strcmp(answer, arr[(i - 1) / 4].options[2]) == 0) {
                arr[(i-1)/4].answer_idx = 3;
            }else {
                printf("this failed %s\n",answer); // if this doesn;t find an answer then return that it failed
            } 
                
         }
        
        if(i % 4 == 0) { // just keep going if its an off line
            continue; 
        }

    }
     return num_questions; // we return the number of questions
    fclose(fp); // close the file
}
void printUsage(char *name) { // this just outputs the general question 
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h] \n", name);
    printf("-f question_file    Default to \"question.txt\"; \n"); 
    printf("-i IP_address       Default to \"127.0.0.1\"; \n");
    printf("-p port_address     Default to 25555; \n");
    printf("-h                  Display this help info.\n");

}

int main(int argc, char* argv[]) {
        struct Player players[MAX_CONN]; // we create a struct with the max players
        char *question_file = "questions.txt"; // standard file
        char *ip_address = "127.0.0.1"; // standard ip address
        int port_number = 25555; // standard port
        int opt; // opt for getopt
        while((opt = getopt(argc, argv, "f:i:p:h")) != -1) { // this checks to see if we are changing a file, ip address, port number of asking for help
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
                    return 0;
                case '?':
                    printf("Error: Unknown option '%c' received. \n", optopt);
                    return 1;
            }
        }
        struct Entry allQuestions[50]; // create a struct
        int questions = read_questions(allQuestions, question_file); // read the questions and fill the struct
        if (questions == -1) { // return an error if it doesnt work
            printf("Error reading questions from file.\n");
            return 1;
        }
        int    server_fd; // creating a sevrer, we got this in class
        int    client_fd;
        struct sockaddr_in server_addr;
        struct sockaddr_in in_addr;
        socklen_t addr_size = sizeof(in_addr);

        /* STEP 1
            Create and set up a socket
        */
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = htons(port_number);
        server_addr.sin_addr.s_addr = inet_addr(ip_address);

        /* STEP 2
            Bind the file descriptor with address structure
            so that clients can find the address
        */
        if ( bind(server_fd,(struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
            perror("Bind"); // check to see if bind failed
            exit(1);
        }
            
        /* STEP 3
            Listen to at most 2 incoming connections
        */
        if (listen(server_fd, 5) == 0)
            printf("Welcome to 392 Trivia!\n"); // greet the clients in the server
        else {
            perror("listen");
            exit(1);
        }

        /* STEP 4
            Accept connections from clients
            to enable communication
        */
        fd_set myset;
        FD_ZERO(&myset);
        FD_SET(server_fd, &myset);
        int maxfd = server_fd;
        int n_conn = 0; // counter for connections
        int iStartGame = 0; // flag for game starting
        int counter = 1; // counter to parse through connections
        
        int cfds[MAX_CONN];
        for (int i = 0; i < MAX_CONN; i++) cfds[i] = -1; // intalize the array

        char* receipt   = "Read\n";
        int   recvbytes = 0;
        char  buffer[1024];
        struct Entry2 firstQuestion[1];
        strcpy(firstQuestion[1].first, allQuestions[0].prompt); // copy the first question before it gets erased
    while (1) {
        // re-initialize fd_set
        FD_ZERO(&myset);
        FD_SET(server_fd, &myset);
        for (int i = 0; i < MAX_CONN; i++) {
            if (cfds[i] != -1) {
                FD_SET(cfds[i], &myset);
                if (cfds[i] > maxfd)
                    maxfd = cfds[i];
            }
        }
        // monitor file descriptor set
        if (select(maxfd + 1, &myset, NULL, NULL, NULL) == -1) {
            perror("Select.");
            exit(1);
        }
       // printf("--dbg--\n");

        // check if new connection is coming in
    
        if (FD_ISSET(server_fd, &myset)) {
            if (n_conn < MAX_CONN) {
                client_fd = accept(server_fd, (struct sockaddr *)&in_addr, &addr_size);
                // n_conn++;
                printf("New connection detected!\n");
                players[n_conn].fd = client_fd;
                players[n_conn].score = 0;
                for (int i = 0; i < MAX_CONN; i++) {
                    if (cfds[i] == -1) {
                        cfds[i] = client_fd;
                        char invalid_name_msg[] = "Please type your name(Name must not be an empty, you will be prompted to put it in again.):";
                        write(cfds[i], invalid_name_msg, strlen(invalid_name_msg));
                        char repeatName[] = "Enter your name again to confirm.";
                       // ssize_t recv_bytes = read(cfds[i], buffer, sizeof(buffer));
                        //write(cfds[i], repeatName, strlen(repeatName));
                        players[i].fd = client_fd;
                        break;
                    }
                }
            } else {
                close(client_fd);
                printf("Max connections reached! \n");
            }
        }
           
            // Receive names from clients
            for (int i = 0; i < MAX_CONN; i++) {
                    if (cfds[i] != -1 && FD_ISSET(cfds[i], &myset)) {
                        ssize_t recv_bytes = read(cfds[i], buffer, sizeof(buffer));
                        if (recv_bytes <= 0) {
                            if (recv_bytes == 0) {
                                printf("%s lost connection\n", players[i].name);
                                printf("Game Terminating... \n");
                                close(server_fd);
                                exit(0);
                            } else {
                                perror("Receive error");
                            }
                            close(cfds[i]);
                            // Remove player from the array
                            for (int j = i; j < MAX_CONN - 1; j++) {    
                                cfds[j] = cfds[j + 1];
                            }
                            n_conn--;
                            cfds[MAX_CONN - 1] = -1;
                        } else if (iStartGame == 0){
                            if(players[i].name[0] != '\0') {
                            
                            } else {
                            n_conn++; 
                            buffer[recv_bytes] = '\0';
                            size_t newline_pos = strcspn(buffer, "\n");
                            // If a newline character is found, replace it with a null terminator
                            if (newline_pos < sizeof(players[i].name)) {
                                buffer[newline_pos] = '\0';
                            }
                            if (buffer[0] == '\0') {
                            // Empty name received, prompt the player to enter a valid name
                            char invalid_name_msg[] = "Please enter a valid name.\n\0";
                            write(cfds[i], invalid_name_msg, strlen(invalid_name_msg));
                            }
                            strncpy(players[i].name, buffer, sizeof(players[i].name));
                            printf("%s joined!\n", players[i].name);
                            char greeting[4096];
                            sprintf(greeting, "Hello %s :):)\n", players[i].name);
                            write(cfds[i],greeting, strlen(greeting));
                            fflush(stdout);
                            if (n_conn == MAX_CONN) {
                                iStartGame = 1;
                                char receipt[1024];
                                sprintf(receipt, "Game started.\n");
                                printf("Game Started!\n");
                                char question_prompt[4096] = {0};
                                printf("Question 1: %s\n 1: %s\n 2: %s\n 3: %s\n\n", firstQuestion[1].first, allQuestions[0].options[0],allQuestions[0].options[1],allQuestions[0].options[2]);
                                sprintf(question_prompt, "Question 1: %s\n Press 1: %s\n Press 2: %s\n Press 3: %s\n", firstQuestion[1].first, allQuestions[0].options[0],allQuestions[0].options[1],allQuestions[0].options[2]);
                                //printf("%s", allQuestions[0].prompt);
                                for (int k = 0; k < MAX_CONN; k++) {
                                    if (cfds[k] != -1) {
                                        write(cfds[k], receipt, strlen(receipt));
                                        write(cfds[k], question_prompt, strlen(question_prompt));
                                    }
                                }
                            }
                        }
                        }

                    else  {
                        
                        if (counter == questions) {
                            char answer[1024];
                            //printf("Answer 1: %s\n", allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                            sprintf(answer, "Answer 1: %s\n\n", allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                            if (atoi(buffer) == allQuestions[counter-1].answer_idx) { 
                                    printf("%s got the point!\n", players[i].name);
                                    players[i].score++;
                                    printf("The current score for: %s is %d\n",players[i].name, players[i].score);
                                    printf("Answer 1: %s\n", allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                                    for (int k = 0; k < MAX_CONN; k++) {
                                        if (cfds[k] != -1) {
                                            write(cfds[k], answer, strlen(answer));
                                        }
                                    }
                            } else {
                                printf("%s lost a point!\n", players[i].name);
                                players[i].score--;     
                                printf("The current score for: %s is %d\n",players[i].name, players[i].score);
                                printf("Answer 1: %s\n\n", allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                                for (int k = 0; k < MAX_CONN; k++) {
                                    if (cfds[k] != -1) {
                                        write(cfds[k], answer, strlen(answer));
                                    }
                                }
                            }
                            int max_score = INT_MIN;
                            // Find the maximum score
                            for (int j = 0; j < MAX_CONN; j++) {
                                if (players[j].score > max_score) {
                                    max_score = players[j].score;
                                }
                            }
                            // Output all players with the maximum score
                            printf("Game over! The winners are:\n");
                            for (int j = 0; j < MAX_CONN; j++) {
                                if (players[j].score == max_score) {
                                    printf("%s with a score of %d\n", players[j].name, max_score);
                                }
                            }
                            // Notify clients about the end of the game and the winners
                            char game_over_msg[1024];
                            sprintf(game_over_msg, "Game over! The winners are:\n");
                            for (int k = 0; k < MAX_CONN; k++) {
                                if (cfds[k] != -1) {
                                    //write(cfds[k], game_over_msg, strlen(game_over_msg));
                                    for (int j = 0; j < MAX_CONN; j++) {
                                        if (players[j].score == max_score) {
                                            char winner_msg[1024];
                                            sprintf(winner_msg, "Game over! The winners are:\n %s with a score of %d\n", players[j].name, max_score);
                                            write(cfds[k], winner_msg, strlen(winner_msg));
                                        }
                                    }
                                }
                            }
                        close(server_fd);
                        exit(0);
                } else {
                            char answer[1024];
                            //sprintf(answer, "%s\n", allQuestions[counter].options[allQuestions[counter].answer_idx - 1]);
                            char question_prompt[4096] = {0};
                           
                            sprintf(question_prompt, "Question %d: %s\n Press 1: %s\n Press 2: %s\n Press 3: %s\n", counter, allQuestions[counter].prompt, allQuestions[counter].options[0],allQuestions[counter].options[1],allQuestions[counter].options[2]);
                            if (atoi(buffer) == allQuestions[counter-1].answer_idx) { 
                                    char answer[1024];
                                    sprintf(answer, "Answer %d: %s\n\n", counter, allQuestions[counter-1].options[allQuestions[counter-1].answer_idx - 1]);
                                    printf("Answer %d: %s\n\n", counter,  allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                                    printf("%s got the point!\n", players[i].name);
                                    players[i].score++;
                                    printf("The current score for: %s is %d\n\n",players[i].name, players[i].score);
                                    printf("Question %d: %s\n 1: %s\n 2: %s\n 3: %s\n\n", counter, allQuestions[counter].prompt, allQuestions[counter].options[0],allQuestions[counter].options[1],allQuestions[counter].options[2]);
                                    for (int k = 0; k < MAX_CONN; k++) {
                                        if (cfds[k] != -1) {
                                            write(cfds[k], answer, strlen(answer));
                                            write(cfds[k], question_prompt, strlen(question_prompt));
                                        }
                                    }
                        } else {
                            char question_prompt[4096] = {0};
                            sprintf(question_prompt, "Question %d: %s\n Press 1: %s\n Press 2: %s\n Press 3: %s\n", counter, allQuestions[counter].prompt, allQuestions[counter].options[0],allQuestions[counter].options[1],allQuestions[counter].options[2]);
                            char answer[1024];
                                sprintf(answer, "Answer %d: %s\n\n", counter, allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                                printf("Answer %d: %s\n", counter, allQuestions[counter -1].options[allQuestions[counter-1].answer_idx - 1]);
                                printf("%s lost a point!\n", players[i].name);
                                players[i].score--;     
                                printf("The current score for: %s is %d\n\n",players[i].name, players[i].score);
                                printf("Question: %s\n 1: %s\n 2: %s\n 3: %s\n\n", allQuestions[counter].prompt, allQuestions[counter].options[0],allQuestions[counter].options[1],allQuestions[counter].options[2]);
                                for (int k = 0; k < MAX_CONN; k++) {
                                if (cfds[k] != -1) {
                                    write(cfds[k], answer, strlen(answer));
                                    write(cfds[k], question_prompt, strlen(question_prompt));
                                }
                            }
                        }
                            counter++;
                        }
                        }
                    }
                }
                
            }
    
   close(server_fd);
    return 0;
}