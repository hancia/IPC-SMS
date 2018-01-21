#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include "const.h"

bool logged=false;
int privateQID;

void Login(int ServerKey, char* name){
    struct Message message,returnmsg;
    printf("Imie: ");
    scanf("%s", &message.sender);
    printf("Haslo: ");
    scanf("%s", &message.message);
    message.type=LOGGING_ATTEMPT_MESSAGE_TYPE;
    msgsnd(ServerKey,&message,MESSAGE_SIZE,0);
    msgrcv(ServerKey,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    if(!strcmp(returnmsg.result,MESSAGE_RESULT_SUCCESS)){
        logged=true;
        printf("Zalogowano, witaj\n",message.sender);
        strcpy(name,message.sender);
        int key = (int) strtol(returnmsg.message, NULL, 10);
        privateQID = msgget(key, IPC_CREAT | 0644);
    }
    else{
        if(!strcmp(returnmsg.message,LOGGING_RESULT_USER_LOGGED)){
            logged=true;
            printf("Jestes juz zalogowany\n");
        }
        else{
            if(!strcmp(returnmsg.message,LOGGING_RESULT_WRONG_PASSWORD))
                printf("Zle haslo, sprobuj jeszcze raz\n");
            else{
                if(!strcmp(returnmsg.message,RESULT_NO_USER))
                    printf("Nie ma takiego uzytkownika\n");
            }
        }
    }
}

void ReceiveMessage(){
    struct Message message;
    while(logged){
        msgrcv(privateQID,&message,MESSAGE_SIZE,MESSAGE_FROM,0);
        printf("Wiadomosc od: %s:\n %s\n",message.sender,message.message);
        printf("\n");
    }
}

void SendMessage(char* name){
    struct Message message, returnmsg;
    char temp_msg[100];
    printf("Odbiorca: \n");
    scanf("%s", message.reciever);
    printf("Tresc:\n");
    scanf("%s",&temp_msg);
    strcpy(message.message,temp_msg);
    message.type=SEND_NORMAL_MESSAGE;
    strcpy(message.sender,name);
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    if(!strcmp(returnmsg.result,MESSAGE_RESULT_ERROR)) printf("Nie udalo sie wyslac wiadomosci, nie ma takiego uzytkownika\n");
}

void Logout(char *name){
    struct Message message,returnmsg;
    strcpy(message.sender,name);
    message.type=LOGOUT_REQUEST;
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    if(!strcmp(returnmsg.result,MESSAGE_RESULT_SUCCESS))
        logged=false;
    exit(0);
}

void JoinGroup(char *name){
    struct Message message,returnmsg;
    char group[20];
    printf("Podaj nazwe grupy: \n");
    scanf("%s", &group);
    message.type=JOIN_GROUP;
    strcpy(message.sender,name);
    strcpy(message.message,group);
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    if(!strcmp(returnmsg.result,MESSAGE_RESULT_ERROR))
        if(!strcmp(returnmsg.message,RESULT_NO_GROUP))
            printf("Nie ma takiej grupy");
        else {
            if(!strcmp(returnmsg.message,RESULT_IN_GROUP))
                printf("Jestes juz w tej grupie");
        }
    else printf("Sukces\n");
}

void ActiveUsers(char* name){
    struct Message message,returnmsg;
    strcpy(message.sender,name);
    message.type=LIST_USER;
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    printf("%s", returnmsg.message);
}

void GroupUsers(char *name){
    struct Message message,returnmsg;
    char group[20];
    printf("Podaj nazwe grupy: \n");
    scanf("%s", &group);
    strcpy(message.sender,name);
    strcpy(message.message,group);
    message.type=LIST_GROUP_USERS;
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    printf("%s", returnmsg.message);
}

void GroupList(char *name){
    struct Message message,returnmsg;
    strcpy(message.sender,name);
    message.type=LIST_GROUPS;
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    printf("%s", returnmsg.message);
}

void MessageGroup(char *name){
    struct Message message, returnmsg;
    char temp_msg[100];
    printf("Grupa: \n");
    scanf("%s", message.reciever);
    printf("Tresc:\n");
    scanf("%s",&temp_msg);
    strcpy(message.message,temp_msg);
    message.type=SEND_MESSAGE_TO_GROUP;
    strcpy(message.sender,name);
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
    if(!strcmp(returnmsg.result,MESSAGE_RESULT_ERROR)) printf("Nie udalo sie wyslac wiadomosci\n");
}

void DeleteFromGroup(char *name){
    struct Message message,returnmsg;
    char group[20];
    printf("Podaj nazwe grupy: \n");
    scanf("%s", &group);
    strcpy(message.sender,name);
    strcpy(message.message,group);
    message.type=LEAVE_GROUP;
    msgsnd(privateQID,&message,MESSAGE_SIZE,0);
    msgrcv(privateQID,&returnmsg,MESSAGE_SIZE,SERVER_RESPONSE,0);
}
int main()
{
    char* name;
    int ServerKey = msgget(serverPublicQueueKey,0644);
    while(!logged){
        Login(ServerKey,name);
    }
    if(fork()==0){//dziecko
        ReceiveMessage();
    }
    else{
        while(logged){
            char command[10];
            fflush(stdout);
            printf("Podaj komende\n");
            scanf("%s", &command);
            if(!strcmp(command,"logout"))
                Logout(name);
            else{
                if(!strcmp(command,"msg"))
                    SendMessage(name);
                else{
                    if(!strcmp(command,"join"))
                        JoinGroup(name);
                    else{
                        if(!strcmp(command,"list"))
                            ActiveUsers(name);
                        else{
                            if(!strcmp(command,"listag"))
                                GroupUsers(name);
                            else
                               if(!strcmp(command,"listg"))
                                   GroupList(name);
                               else{
                                   if(!strcmp(command,"msgg"))
                                       MessageGroup(name);
                                   else{
                                       if(!strcmp(command,"leave"))
                                           DeleteFromGroup(name);
                                }
                            }
                        }
                    }
                }
                
            }
        }
    }
    return 0;
}
