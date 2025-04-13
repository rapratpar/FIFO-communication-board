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
#include <stdbool.h>
#include <signal.h>

const int USERNAME_LENGTH = 16;
const int USERS_AMOUNT = 20;
const int THEMES_AMOUNT = 20;
const int THEME_LENGTH = 16;
const int COMMUNICATION_LENGTH = 255;
const int MESSAGES_TYPES = 8;

int users_number = 0;
int themes_number = 0;

int ID;
const int KEY = 1234;
bool running = true;

struct User {
    int ID;
    int msgID;
    char nick[16];
    int themes[20];
};

struct Theme {
    int ID;
    char title[16];
    int users[20];
};

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

struct Communication COMS, COMR;
struct User users[20];
struct Theme themes[20];


bool is_unique_name(char *nick) {
    for (int i = 0; i < users_number; i++) {
        if (strcmp(nick, users[i].nick) == 0) {
            return false;
        }
    }
    return true;
}

bool is_unique_title(char *title) {
    for (int i = 0; i < themes_number; i++) {
        if (strcmp(title, themes[i].title) == 0) {
            return false;
        }
    }
    return true;
}

int findTheme(int id, int msg_id, char *title) {
    COMS.mtype = id;
    COMS.msgID = msg_id;

    for (int i = 0; i < themes_number; i++) {
        if (strcmp(title, themes[i].title) == 0) {
            COMS.type = 10;
            // Convert integer i to string and store in COMS.text
            sprintf(COMS.text, "%d", i);
            return i;
        }
    }
    COMS.type = -1;
    strcpy(COMS.text, "This theme was not found. \n");
    return -1;
}


int findUser(int id) {
    for (int i = 0; i < users_number; i++) {
        if (users[i].ID == id) {
            return i;
        }
    }
    return -1;
}

void addNewUser(int id, int msg_id, char *name) {
    int check = true;
    COMS.mtype = id;
    COMS.msgID = msg_id;
    if (users_number < USERS_AMOUNT) {
        if (is_unique_name(name)) {
            users[users_number].ID = id;
            users[users_number].msgID = msg_id;
            strcpy(users[users_number].nick, name);
            users_number++;
            COMS.type = 10;
            return;
        } else {
            strcpy(COMS.text, "This nick is taken - can't add user.\n");
            COMS.type = -1;
            return;
        }
    } else {
        strcpy(COMS.text, "Server is full - can't add any users.\n");
        COMS.type = -1;
        return;
    }
}
void addNewTopic(int id, int msg_id, char *title) {
    COMS.mtype = id;
    if (themes_number < THEMES_AMOUNT) {
        if (is_unique_title(title)) {
            themes[themes_number].ID = themes_number + 1;
            strcpy(themes[themes_number].title, title);
            themes[themes_number].users[0] = msg_id;
            COMS.mtype = msg_id;
            COMS.type = 3;
            COMS.subs_type = -1;
            strcpy(COMS.theme, title);
            strcpy(COMS.text, "Theme was successfully added. \n");

            msgsnd(ID, &COMS, sizeof(COMS), 0);

            themes_number++;
            COMS.type = 10;
            COMS.mtype = id;

        } else {
            strcpy(COMS.text, "This theme already exists - can't add a new theme.\n");
            COMS.type = -1;
        }
    } else {
        strcpy(COMS.text, "Server is full - can't add any new theme.\n");
        COMS.type = -1;
    }

}

void printUsers(int id) {
    COMS.mtype = id;
    strcpy(COMS.text, "Users:\n");
    for (int i = 0; i < users_number; i++) {
        char user_info[100];
        if(users[i].ID!=0){
            sprintf(user_info, "ID: %d, Nick: %s\n", users[i].ID, users[i].nick);
            strcat(COMS.text, user_info);
        }
    }
}

void printThemes(int id) {
    COMS.mtype = id;
    strcpy(COMS.text, "Themes:\n");
    for (int i = 0; i < themes_number; i++) {
        char theme_info[100];
        sprintf(theme_info, "ID: %d, Title: %s\n", themes[i].ID, themes[i].title);
        strcat(COMS.text, theme_info);
    }
}

void addSubscriber(int id, int msg_id, char *title, int amount) {
    int flag1 = 0, flag2 = 0;
    int themeID;
    COMS.mtype = id; //?
    COMS.msgID = msg_id;

    for (int i = 0; i < themes_number; i++) {
        if (strcmp(themes[i].title, title) == 0) {
            //Temat znaleziony
            for (int j = 0; j < users_number; j++) {
                //Zapisujemy użytkownika na pierwsze wolne pole
                if (themes[i].users[j] == 0 || themes[i].users[j] == msg_id) {
                    themes[i].users[j] = msg_id;
                    COMS.type = 3;
                    COMS.mtype = msg_id;
                    COMS.subs_type = amount;
                    strcpy(COMS.theme, title);
                    msgsnd(ID, &COMS, sizeof(COMS), 0);
                    flag2 = 1;
                    COMS.mtype = id;

                    break;
                }
            }
            themeID = i;
            flag1 = 1;
            break;
        }
    }

    if (flag1 == 1 && flag2 == 1) {
        sprintf(COMS.text, "%d ; User was successfully registered on theme.\n", themeID);
        COMS.type = 10;
    } else {
        if (flag1 == 0) {
            strcpy(COMS.text, "Registration failed. Theme was not found. \n");
        } else if (flag2 == 0 && flag1 != 0) {
            strcpy(COMS.text, "Registration failed. User was not found. \n");
        }
        COMS.type = -1;
    }
}

void removeSubscriber(int id, int msg_id, char *title) {
    int flag1 = 0, flag2 = 0;
    int themeID;
    COMS.mtype = id;
    COMS.msgID = msg_id;

    for (int i = 0; i < themes_number; i++) {
        if (strcmp(themes[i].title, title) == 0) {
            for (int j = 0; j < users_number; j++) {
                if (themes[i].users[j] == msg_id) {
                    themes[i].users[j] = 0;
                    //znaleziono uzytkownika
                    flag2 = 1;
                    break;
                }
            }
            themeID = i;
            //znaleziono temat
            flag1 = 1;
            break;
        }
    }

    if (flag1 == 1 && flag2 == 1) {
        sprintf(COMS.text, "Subscribtion has ended. \n", themeID);
        COMS.type = 10;
    } else {
        if (flag1 == 0) {
            strcpy(COMS.text, "Subscriber was not removed. Theme was not found. \n");
        } else if (flag2 == 0 && flag1 != 0) {
            strcpy(COMS.text, "Subscriber was not removed. User was not found. \n");
        }
        COMS.type = -1;
    }
}
int checkIfRegistered(int msg_id, char *theme){
    for (int i = 0; i < themes_number; i++) {
        if (strcmp(themes[i].title, theme) == 0) {
            for (int j = 0; j < users_number; j++) {
                //nie wysylam wiadomosci do samego siebie
                if (themes[i].users[j] == msg_id) {
                    return 0;
                }
            }
        }
    }
    return 1;
}
void receiveAndPassMessage(int id, int msg_id,  char *text, char *theme, int priority) {
    if(checkIfRegistered(msg_id, theme) == 0){
        char title[16]; //THEME_LENGTH
        char name[16]; //USERNAME_LENGTH
        char message[255]; //MESSAGE_LENGTH
        int flag = 0;

        COMS.userID = id;
        COMS.msgID = msg_id;
        COMS.priority = priority;
        //Szukamy nicku uzytkownika na podstawie jego nicku
        int userID_tab = findUser(id);
        strcpy(name,users[userID_tab].nick);

        //wiadomosc wysylamy w posyaci name;text =  name;priority;text
        strcpy(COMS.text, name);
        strcat(COMS.text, ";");
        strcat(COMS.text, text);

        strcpy(COMS.theme, theme);

        for (int i = 0; i < themes_number; i++) {
            if (strcmp(themes[i].title, theme) == 0) {
                for (int j = 0; j < users_number; j++) {
                    printf("%d", themes[i].users[j]);
                    //nie wysylam wiadomosci do samego siebie
                    if (themes[i].users[j] > 0 && themes[i].users[j] != msg_id) {
                        COMS.type = 1;
                        COMS.mtype = themes[i].users[j];
                        msgsnd(ID, &COMS, sizeof(COMS), 0);
                        flag++;
                    }
                }
            }
        }
        //wiadomosc zwrotna dla procesu ktory wysyla wiadomosc (Wliczamy wyslanie do uzytkownikow ktorzy zbanowali sendera)
        COMS.type = 11;
        COMS.mtype = msg_id;
        sprintf(COMS.text, "Message was successfully sent to %d users.\n", flag);
        msgsnd(ID, &COMS, sizeof(COMS), 0);
        return;
    }else{
        COMS.type = -1;
        COMS.mtype = msg_id;
        strcpy(COMS.text, "You are not registered to this theme. \n");
        msgsnd(ID, &COMS, sizeof(COMS), 0);
        return;
    }
}

void display_msg(int id, int msg_id, char *title, char *text) {
    COMS.type = 2;
    COMS.mtype = msg_id;
    strcpy(COMS.text, text); //ilosc wiadomosci do odczytu
    strcpy(COMS.theme, title);
    msgsnd(ID, &COMS, sizeof(COMS), 0);
    return;
}

void block(int id, int msg_id, char *name) {
    for(int i = 0; i<USERS_AMOUNT; i++) {
        if(!strcmp(users[i].nick, name)) {
            COMS.mtype = msg_id;
            COMS.msgID = msg_id;
            COMS.userID = id;
            printf("uido %d\n",users[i].ID);
         //   sprintf(COMS.text,"%d", users[i].ID);
            strcpy(COMS.text, name);
            COMS.type = 5;
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            COMS.mtype = id;
            strcpy(COMS.text, "User was blocked");
            COMS.type = 1;
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            return;
        }
    }
    COMS.msgID = id;
    strcpy(COMS.text, "Nie znaleziono uzytkonika");
    COMS.type = -1;
    msgsnd(ID, &COMS, sizeof(COMS), 0);
    return;
}

void clearTab(int *tab){
    for(int i = 0; i < THEMES_AMOUNT; i++)
        tab[i] = 0;
    return;
}

void deleteUser(int id, int msg_id){
    int flag2 = 0;
    int themeID;
    COMS.mtype = id;
    COMS.msgID = msg_id;

    for (int i = 0; i < themes_number; i++) {
            // found theme
            for (int j = 0; j < users_number; j++) {
                if (themes[i].users[j] == msg_id) {
                    themes[i].users[j] = 0;
                    flag2 = 1;
                    break;
                }
            }
    }
    int foundID = findUser(id);
    if(foundID >= 0){
        users[foundID].ID = 0;
        strcpy(users[foundID].nick,"");
        clearTab(users[foundID].themes);
        users[foundID].msgID = 0;
        flag2++;
    }
    if (flag2 == 2) {
        sprintf(COMS.text, "User was removed. \n");
        COMS.type = 10;
    } else {
        if (flag2 == 0) {
            strcpy(COMS.text, "User was not removed. User was not found. \n");
        }
        COMS.type = -1;
    }
    return;
}
int main() {
    fflush(stdout);
    ID = msgget(1234, 0666 | IPC_CREAT);
    long rcv;
    while (running) {
        rcv = msgrcv(ID, &COMR, sizeof(COMR), 1, 0);
        switch (COMR.type) {
        case 1: {
            printf("Logging in\n");
            addNewUser(COMR.userID, COMR.msgID, COMR.text);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 2: {
            printf("Registering a topic\n");
            addNewTopic(COMR.userID, COMR.msgID, COMR.text);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 3: {
            printf("Displaying all users\n");
            printUsers(COMR.userID);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 4: {
            printf("Displaying all topics\n");
            printThemes(COMR.userID);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 5: {
            printf("Receiving a message\n");
            receiveAndPassMessage(COMR.userID, COMR.msgID, COMR.text, COMR.theme, COMR.priority);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 6: {
            printf("Registering a subscription\n");
            addSubscriber(COMR.userID, COMR.msgID, COMR.theme, COMR.subs_type);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 7: {
            printf("Searching for a topic\n");
            findTheme(COMR.userID, COMR.msgID, COMR.theme);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 8: {
            printf("Removing a subscription\n");
            removeSubscriber(COMR.userID, COMR.msgID, COMR.theme);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 9: {
            printf("Displaying messages\n");
            display_msg(COMR.userID, COMR.msgID, COMR.theme, COMR.text);
            break;
        }

        case 10: {
            printf("Create a blacklist\n");
            block(COMR.userID, COMR.msgID, COMR.text);
            break;
        }

        case 11: {
            printf("Show subscriptions\n");
            COMS.mtype = COMR.msgID;
            COMS.type = 4;
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }

        case 12: {
            printf("Delete user\n");
            deleteUser(COMR.userID, COMR.msgID);
            msgsnd(ID, &COMS, sizeof(COMS), 0);
            break;
        }
        }
    }
    return 0;
}
