#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

const int KEY = 1234;
const int USERNAME_LENGTH = 16;
const int USERS_AMOUNT = 20;
const int THEMES_AMOUNT = 20;
const int THEME_LENGTH = 16;
const int COMMUNICATION_LENGTH = 255;
int ID;
int ParentID;
char block_list[20][20]; //users_amount
int blocked_amount = 0;
char username[16];
struct Communication {
    long mtype;
    int type;
    int userID;
    char theme[16];
    char text[255];
    int msgID;
    int subs_type;
    int priority;
};

typedef struct queue_t {
    int userID;
    char theme[16];
    char text[255];
    char nick[16];
    int subs_type;
    int priority;
    struct queue_t* next;
} queue_t;

queue_t* list_of_queues[20];
struct Communication sendingComs, receivingComs;
//functions for queue
void init_list(queue_t* list_of_queues[20]) {
    for(int i=0;i<20;i++) {
        list_of_queues[i] = malloc(sizeof(queue_t));
        list_of_queues[i]->next = NULL;
        strcpy(list_of_queues[i]->text, "");
        strcpy(list_of_queues[i]->theme, "");
        list_of_queues[i]->priority = 0;
        list_of_queues[i]->next = NULL;
        list_of_queues[i]->userID = -1;
    };
}

void add_to_queue(queue_t** startQ, char mtext[255], char nick[16], int priority) {
    // Allocate memory for the new node
    queue_t* newQ = malloc(sizeof(queue_t));

    if (newQ == NULL) {
        printf("Queue: No space available\n");
        return;
    }

    // Copy the data to the new node
    strcpy(newQ->text, mtext);
    strcpy(newQ->nick, nick);
    newQ->priority = priority;
    newQ->next = NULL;
    queue_t* currQ = *startQ;
    // If the queue is empty or the new node has higher priority than the first node
    //printf("Piorytet: %d\n", priority);

    // Find the appropriate position to insert the new node
    while (currQ->next != NULL && currQ->next->priority <= priority) {
        //printf("Zmienimay noda na kolejny\n");
        currQ = currQ->next;
    }
    if (currQ->next == NULL) {
        //printf("Dodano jako ostantie\n");
        currQ->next = newQ;
        return;
    }
    //printf("Robimy podmiane\n");

    // Insert the new node
    newQ->next = currQ->next;
    currQ->next = newQ;
}

void delete_first(queue_t** startQ) {
    queue_t* start = *startQ;

   if(start == NULL || start->next == NULL) {
        return;
    };

   queue_t* firstQ = start->next;
   start->next = firstQ->next;

   free(firstQ);
}

int check_if_exists(queue_t* list_of_queues[20], char theme[16]) {
    // sprawdzamy czy podany temat jest gdzie w kolejce
    for(int i=0;i<20;i++) {
        if (!strcmp(list_of_queues[i]->theme, theme)) {
                return i;
        }
    }
    return -1;
}

int get_queue_length(queue_t* list_of_queues[20], int index) {
    if (index >= 0 && index < THEMES_AMOUNT) {
        queue_t* current_queue = list_of_queues[index];
        int length = 0;

        while (current_queue != NULL) {
            length++;
            current_queue = current_queue->next;
        }

        return length;
    } else {
        // Invalid index
        return -1;
    }
}

void add_msg(queue_t* list_of_queues[20], char theme[16], char mtext[255], char nick[16], int userID, int priority) {
    int index = check_if_exists(list_of_queues, theme);
    // sprawdzamy czy temat istenije
    if(index != -1) {
        if(list_of_queues[index]->subs_type > 0) {
      //      printf("Temporary subscription for reading remaining %d\n", list_of_queues[index]->subs_type - 1);
            list_of_queues[index]->subs_type = list_of_queues[index]->subs_type - 1;
           // printf("piorytet w msg: %d\n", priority);
            add_to_queue(&list_of_queues[index], mtext, nick, priority);
            if(list_of_queues[index]->subs_type == 0){
                printf("Temporary subscription of %s has ended, all messages have been received\n", theme);
            }
            return;
        }
        if(list_of_queues[index]->subs_type == 0) {
            //printf("Temporary subscription of %s has ended, all messages have been received\n", theme);
            return;
        }
        if(list_of_queues[index]->subs_type == -1) {
          //  printf("Subscription pernament, message has been added\n");
            //printf("piorytet w msg: %d\n", priority);
            add_to_queue(&list_of_queues[index], mtext, nick, priority);
            return;
        }
    } else {
        printf("Queue: Topic not found\n");
    }
}

void msg_rcv(queue_t* list_of_queues[20], char theme[16]) {
    int index = check_if_exists(list_of_queues, theme);
    if(index != -1) {
        // sprawdzamy czy temat istenije
        if(list_of_queues[index]->subs_type > 0) {
             //   printf("Temporary subscription for reading remaining %d\n", list_of_queues[index]->subs_type - 1);
            list_of_queues[index]->subs_type = list_of_queues[index]->subs_type - 1;
            if(list_of_queues[index]->subs_type == 0){
                printf("Temporary subscription of %s has ended, all messages have been received\n", theme);

            }
            return;
        }
        if(list_of_queues[index]->subs_type == 0) {
            printf("Temporary subscription of %s has ended, all messages have been received\n", theme);
            return;
        }
        if(list_of_queues[index]->subs_type == -1) {
            return;
        }

    } else {
        printf("Queue: Topic not found\n");
    }
}

void add_theme(queue_t* list_of_queues[20], char theme[16], int subs_type) {
    int themeID = check_if_exists(list_of_queues, theme);
    if(themeID!= -1) {
        //modyfikuje temat, subskrypcje
        list_of_queues[themeID] -> subs_type = subs_type;
        return;
    }
    for(int i=0;i<20;i++) {
          if(!strcmp(list_of_queues[i]->theme, "")) {
              strcpy(list_of_queues[i]->theme, theme);
              list_of_queues[i]->subs_type = subs_type;
              return;
            }
        }
    return;
}

void displayAllQueues(queue_t* list_of_queues[20], int type) {
    // sprawdzamy czy podany temat jest gdzie w kolejce
    if(type == 1){
    for(int i=0;i<20;i++) {
        if(list_of_queues[i]->subs_type == -1)
            printf("%s pernament subscription\n" , list_of_queues[i]->theme);
        else if(list_of_queues[i]->subs_type > 0 )
            printf("%s left %d messages to recive\n" , list_of_queues[i]->theme, list_of_queues[i]->subs_type);
        }
    }else{
        for(int i=0;i<20;i++) {
        if(list_of_queues[i]->subs_type == -1){
            printf("%s pernament subscription you can display %d messages\n" , list_of_queues[i]->theme, get_queue_length(list_of_queues, i)-1);
        }
        else if(list_of_queues[i]->subs_type > 0 ) {
            printf("%s left %d messages to read, you can display %d messages\n" , list_of_queues[i]->theme, list_of_queues[i]->subs_type, get_queue_length(list_of_queues, i)-1);
        } else if(list_of_queues[i]->subs_type == 0 && strcmp(list_of_queues[i]->theme, "")) {
            printf("%s: you have recived all messages, you can display %d messages\n" , list_of_queues[i]->theme, get_queue_length(list_of_queues, i)-1);
        }
    }
    };
    return;
}

void display_message(queue_t* list_of_queues[20], char theme[16], char text[255]) {
    int index = check_if_exists(list_of_queues, theme);
    if(index != -1) {
            int number = atoi(text);
            printf("%d\n", number);
            printf("=======%s========\n", theme);
            while(number != 0 && list_of_queues[index]->next != NULL) {
                printf("%s - %s\n",&list_of_queues[index]->next->nick , &list_of_queues[index]->next->text);
                delete_first(&list_of_queues[index]);
                number--;
            };
            if(list_of_queues[index]->next == NULL && list_of_queues[index]->subs_type == 0) {
                sendingComs.mtype = 1;
                sendingComs.type = 8; //remove subscription
                sendingComs.msgID = ParentID;
                strcpy(sendingComs.theme, theme);
                strcpy(list_of_queues[index]->theme, "");
                int sendStatus = msgsnd(ID, &sendingComs, sizeof(sendingComs), 0);
            };
        printf("===============\n", theme);
        return;
    }
    printf("Topic not found\n");
    return;
}

void Menu() {
    printf("MENU:\n");
    printf("0. Display menu\n");
    printf("1. Register in Broadcasting System\n");
    printf("2. Display List of Available Topics\n");
    printf("3. Add New Topic\n");
    printf("4. Send Message\n");
    printf("5. Display List of All Users\n");
    printf("6. Add User to Blacklist\n");
    printf("7. Show subscriptions\n");
    printf("8. Recive messages \n");
    printf("9. Exit\n");
    printf("Choose an option (0-9): \n");
}
int logIn(int ID, int userPID) {
    printf("Input your username: \n");
    scanf("%s", username); // Limit input to avoid buffer overflow
    sendingComs.mtype = 1; //serwer
    sendingComs.type = 1; // LOGIN
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;
    strcpy(sendingComs.text, username);
    int sendStatus = msgsnd(ID, &sendingComs, sizeof(struct Communication), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
        return 1; // Return an error code
    }
    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(struct Communication), userPID, 0);
    if (receivingComs.type == -1) {
        printf("%s\n", receivingComs.text);
        return 1; // Return an error code
    } else {
        return 0; // Return success code
    }
}
int addNewTopic(int ID, int userPID) {
    char title[20];
    fflush(stdout);
    printf("Input title of new topic: \n");
    scanf("%s", title);
    sendingComs.mtype = 1;
    sendingComs.type = 2; // register a topic
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;

    strcpy(sendingComs.text, title);
    int sendStatus = msgsnd(ID, &sendingComs, sizeof(struct Communication), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
        return 1; // Return an error code
    }
    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(struct Communication), userPID, 0);
    if (receivingComs.type == -1) {
        printf("%s\n", receivingComs.text);
        return 1; // Return an error code
    } else {
        printf("%s\n", receivingComs.text);
        return 0; // Return success code
    }
}
int displayListOfAllUsers(int ID, int userPID) {
    sendingComs.mtype = 1;
    sendingComs.type = 3; // display users
    sendingComs.userID = userPID;
    int sendStatus = msgsnd(ID, &sendingComs, sizeof(sendingComs) - sizeof(long), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
    }
    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(receivingComs), userPID, 0);
    if (receivingComs.type == -1) {
        printf("%s\n", receivingComs.text);
    } else {
        printf("%s\n", receivingComs.text);
        return 0;
    }
}

int displayListOfAvailableTopics(int ID, int userPID) {
    sendingComs.mtype = 1;
    sendingComs.type = 4; // 4 - display topics
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;

    int sendStatus = msgsnd(ID, &sendingComs, sizeof(struct Communication), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
        return 0;
    }
    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(receivingComs), userPID, 0);
    if (receivingComs.type == -1) {
        printf("%s\n", receivingComs.text);
        return 0;
    } else {
        if(!strcmp("Themes:\n",receivingComs.text)) {
            printf("No topics avaliable\n");
            return 1; //error
        }
        printf("%s\n", receivingComs.text);
        return 0;
    }
}

void sendMessage(int ID, int userPID) {
    char theme[16];
    char message[255]={"\0"};
    int priority;
    displayListOfAvailableTopics(ID, userPID);
    printf("Input the topic of your message: \n");
    scanf("%s", theme);

    printf("Input your message: \n");
	scanf(" %[^\n]",message);
    printf("Input the priority of your message: \n");
    scanf("%d", &priority);

    //printf("piorytet na poczatku: %d\n", priority);

    sendingComs.mtype = 1;
    sendingComs.type = 5; // send message
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;
    strcpy(sendingComs.theme, theme);
    strcpy(sendingComs.text, message);
    sendingComs.priority = priority;

    int sendStatus = msgsnd(ID, &sendingComs, sizeof(struct Communication), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
        return;
    }
}

void registerInBroadcastingSystem(int ID, int userPID) {
    char title[16]; //THEME_LENGTH
    if(displayListOfAvailableTopics(ID, userPID) == 1) return;
    printf("Choose a topic to join:\n");
    scanf("%s", title);
    sendingComs.mtype = 1;
    sendingComs.type = 6; // register subscription
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;
    strcpy(sendingComs.theme, title);

    printf("Choose subscription type:\n");
    printf("0 for temporary, another number for permanent: \n");
    int subscriptionType;
    scanf("%d", &subscriptionType);

    if(subscriptionType == 0) {
        int messagesAmount;
        printf("Input how many messages you want to receive: \n");
        scanf("%d", &messagesAmount);
        strcpy(sendingComs.text, "temp");
        sendingComs.subs_type = messagesAmount;
    } else {
        strcpy(sendingComs.text, "const");
        sendingComs.subs_type = -1;
    };

    int sendStatus = msgsnd(ID, &sendingComs, sizeof(sendingComs), 0);
    if (sendStatus == -1) {
        perror("Error: Message send\n");
        return; // Return an error code
    }

    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(receivingComs), userPID, 0);
    if (receivingComs.type == -1) {
        printf("%s\n", receivingComs.text);
    } else if (receivingComs.type == 10) {
    }
}

void displaySubscriptions(int ID, int userPID){
    sendingComs.mtype = 1;
    sendingComs.type = 11; // display Subscriptions
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;
    int sendStatus = msgsnd(ID, &sendingComs, sizeof(struct Communication), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
        return;
    }
    return;
}

int getThemeID(int ID, int userPID, char *theme){
    sendingComs.mtype = 1;
    sendingComs.type = 7; //find Theme ID
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;
    int sendStatus = msgsnd(ID, &sendingComs, sizeof(sendingComs), 0);
    if (sendStatus == -1) {
        perror("Error: Message send\n");
        return -1; // Return an error code
    }
    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(struct Communication), userPID, 0);
    if (receivingComs.type == -1) {
        return -1;
    } else {
        return atoi(receivingComs.text);
    }
}
void reciveMessages(int ID, int userPID){
    displaySubscriptions(ID, userPID);

    printf("Which theme you want to read?\n");
    char title[16]; //THEME_LENGTH
    scanf("%s", title);
    printf("How many messages you want to recive? \n");
    int number;
    scanf("%d", &number);
    sendingComs.mtype = 1;
    sendingComs.type = 9; //display mesages
    strcpy(sendingComs.theme, title);
    sprintf(sendingComs.text, "%d", number);
    msgsnd(ID, &sendingComs, sizeof(sendingComs), 0);
}

// BLOKOWANIE ---------------------
void sendBlock(int ID, int userPID) {
    displayListOfAllUsers(ID, userPID);
    //printf("subscription send block:\n");
    //displaySubscriptions(ID, userPID);
    printf("Which user you want to black list. \n");
    char blocked[16];
    scanf("%s", blocked);
    if(!strcmp(blocked, username)) {
        printf("You cannot block yourself\n");
        return;
    }
    sendingComs.mtype = 1;
    sendingComs.type = 10;
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;
    strcpy(sendingComs.text, blocked);
    msgsnd(ID, &sendingComs, sizeof(sendingComs), 0);
    return;
};

void addBlock(char *blUser) {
    //printf("subscription ad block:\n");
    //displayAllQueues(list_of_queues, 1);
    //printf("add block to: %s\n", blUser);
    if(blocked_amount == 20) {
        printf("Przekroczono limit blokowania\n");
        return;
    }
    for(int i=0;i<blocked_amount;i++){
        if(!strcmp(block_list[i], blUser)) {
            printf("User is already blocked \n");
            return;
        };
    }
    strcpy(block_list[blocked_amount], blUser);
    printf("Blocked %s\n", blUser);
    blocked_amount++;
}

int check_if_blocked(char *blUser) {
    for(int i=0;i<blocked_amount;i++) {
        if(!strcmp(block_list[i], blUser)) {
            return 1;
        }
    }
    return 0;
}
void deleteUsr(int ID, int userPID){
    sendingComs.mtype = 1;
    sendingComs.type = 12; // 12 - delete user
    sendingComs.userID = userPID;
    sendingComs.msgID = ParentID;

    int sendStatus = msgsnd(ID, &sendingComs, sizeof(struct Communication), 0);
    if (sendStatus == -1) {
        perror("Error : Message send\n");
    }
    int receiveStatus = msgrcv(ID, &receivingComs, sizeof(receivingComs), userPID, 0);
    if (receivingComs.type == -1) {
        printf("%s\n", receivingComs.text);
    } else {
        printf("%s\n", receivingComs.text);
    }

}
void running(int type){
    ParentID = getpid();

    int userPID = fork();
    int temp = 1;
    int option;
    long statusM;
    int safe; //check if option was digit
    if (userPID > 0) {

        while ((temp = logIn(ID, userPID)) != 0) {

            if (temp == 1) {
                printf("Login failed. Please try again.\n");
            } else {
                break;
            }
        }
        printf("Ready.\n");
        Menu();
        while (1) {
            printf("OPTION: ");
            fflush(stdout);
            safe = scanf("%d", &option);
            if (safe != 1 || getchar() != '\n') {
                option = 0;
                while (getchar() != '\n');
            }
            switch (option) {
                case 0:
                    Menu();
                    break;
                case 1:
                    registerInBroadcastingSystem(ID, userPID);
                    break;
                case 2:
                    displayListOfAvailableTopics(ID, userPID);
                    break;
                case 3:
                    addNewTopic(ID, userPID);
                    break;
                case 4:
                    sendMessage(ID, userPID);
                    break;
                case 5:
                    displayListOfAllUsers(ID, userPID);
                    break;
                case 6:
                    sendBlock(ID, userPID);
                    break;
                case 7:
                    displaySubscriptions(ID, userPID);
                    break;
                case 8:
                    if(type == 0) reciveMessages(ID, userPID);
                    else printf("This option is avaliable only in synchronous mode. \n");
                    break;
                case 9:
                    // Exit the program
                    deleteUsr(ID, userPID);
                    printf("Exiting\n");
                    exit(0); // Terminate the program
                    break;
                default:
                    printf("Invalid option. Please choose a valid option.\n");
                    break;
                }
        }
    }else{
        char *token;;
        char nick[16]; // USERNAME_LENGTH
        char text[255]; // MESSAGE_LENGTH
        while(1){
            statusM =  msgrcv(ID, &receivingComs, sizeof(receivingComs), ParentID, 0);
            if(statusM > 0 ){
                if(type == 1){
                    //printing message asynchronous
                    token = strtok(receivingComs.text, ";");
                    switch(receivingComs.type) {
                        case 1:
                            if (token != NULL) {
                                strcpy(nick, token);
                                token = strtok(NULL, ";");
                                if (token != NULL) {
                                    strcpy(text, token);
                                    if (check_if_blocked(nick) == 0) {
                                        printf("===== TOPIC : %s =====\n %s - %s\n",receivingComs.theme, nick, text);
                                        msg_rcv(list_of_queues, receivingComs.theme); // zmniejszam liczbe wiadomosci do
                                        printf("==========\n");
                                    }else{
                                        printf("Message from blocked user %s\n", nick);
                                    }
                                }
                            }
                            break;
                        case 3:
                            if(receivingComs.type != -1){
                                add_theme(list_of_queues, receivingComs.theme, receivingComs.subs_type);
                                if(receivingComs.subs_type == -1) {
                                   // printf("CONST\n");
                                } else {
                                  //  printf("Ammount: %d\n", receivingComs.subs_type);
                                }
                            }
                            break;
                        case 4:
                            displayAllQueues(list_of_queues, type);
                            break;
                        case 5:
                            addBlock(receivingComs.text);
                            break;
                        case -1:
                            printf("%s\n", receivingComs.text);
                            break;
                    }
                }else{
                    switch(receivingComs.type) {
                        case 1:
                            token = strtok(receivingComs.text, ";");
                            if (token != NULL) {
                                strcpy(nick, token);
                                token = strtok(NULL, ";");
                                if (token != NULL) {
                                    strcpy(text, token);
                                    if (check_if_blocked(nick) == 0) {
                                        add_msg(list_of_queues, receivingComs.theme, text, nick, receivingComs.userID, receivingComs.priority);
                                    }
                                }
                            }
                            break;
                        case 2:
                            display_message(list_of_queues, receivingComs.theme, receivingComs.text);
                            break;
                        case 3:
                            //dodanie nowego tematu przez użytkownika
                            if(receivingComs.type != -1){
                                add_theme(list_of_queues, receivingComs.theme, receivingComs.subs_type);
                                if(receivingComs.subs_type == -1) {
                                 //   printf("CONST\n");
                                } else {
                                  //  printf("Ammount: %d\n", receivingComs.subs_type);
                                }
                            }
                            break;
                        case 4:
                            displayAllQueues(list_of_queues, type);
                            break;
                        case 5:
                            addBlock(receivingComs.text);
                            break;
                        case -1:
                            printf("%s\n", receivingComs.text);
                            break;
                    }
                }
            }
        }
    }
}
//type 1 ansynchronous, 0 synchronous

int main(int argc, char *argv[]) {
    ID = msgget(KEY, 0666 | IPC_CREAT);

    if (ID < 0) {
        perror("Error: creating or getting msgID goes wrong\n");
        exit(1);
    }

    init_list(list_of_queues);

    running(atoi(argv[1]));
    return 0;
}
