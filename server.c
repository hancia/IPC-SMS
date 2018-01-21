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
#include "const.h"

#define NumberofUsers 9
#define NumberofGroups 3

const int queueIdOffset = 2223411;
int flag=1;

struct User{
    char name[20];
    char password[20];
    int queueKey;
    int queueId;
    bool logged;
};
struct User Users[NumberofUsers];

struct Group {
    char name[20];
    struct User user[20];
    int usersInGroup;
};
struct Group Groups[NumberofGroups];

void read_users(){
    FILE *file = fopen("users","r");
    int i=0;
    while(!feof(file)){
        fscanf(file,"%s",Users[i].name);
        fscanf(file,"%s",Users[i].password);
        Users[i].queueKey = queueIdOffset + i;
        Users[i].queueId = msgget(Users[i].queueKey, IPC_CREAT | 0644);
        if(flag==0) msgctl(Users[i].queueId,IPC_RMID,0);
        Users[i].logged=false;
        i++;
    }
    fclose(file);
}

void read_groups(){
    FILE *f = fopen("groups","r");
    int i=0;
    while(!feof(f)&&i<NumberofGroups){
        fscanf(f,"%s",Groups[i].name);
        Groups[i].usersInGroup=0;
        i++;
    }
    fclose(f);
}

int FindUser(struct Message *message){
    int i;
    for(i=0; i<NumberofUsers; i++){
        if(!strcmp(Users[i].name,(*message).sender))
            return i;
    }
    return -1;
}

void Login(struct Message *message, int ServerKey){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    int i=FindUser(message);
    if(i!=-1){
        if(!strcmp(Users[i].password,(*message).message)){
            if(!Users[i].logged){
                Users[i].logged=true;
                strcpy(returnmsg.result, MESSAGE_RESULT_SUCCESS);
                sprintf(returnmsg.message, "%d", Users[i].queueKey);
            }
            else{
                strcpy(returnmsg.result, MESSAGE_RESULT_ERROR);
                strcpy(returnmsg.message, LOGGING_RESULT_USER_LOGGED);
            }
        }
        else{
            strcpy(returnmsg.result, MESSAGE_RESULT_ERROR);
            strcpy(returnmsg.message, LOGGING_RESULT_WRONG_PASSWORD);
        }
    }
    else{
        strcpy(returnmsg.result, MESSAGE_RESULT_ERROR);
        strcpy(returnmsg.message, RESULT_NO_USER);
    }
   msgsnd(ServerKey,&returnmsg,MESSAGE_SIZE,0);
}

void Logout(struct Message message,int ServerKey){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    int i=FindUser(&message);
    Users[i].logged=false;
    strcpy(returnmsg.result,MESSAGE_RESULT_SUCCESS);
    msgsnd(ServerKey,&returnmsg,MESSAGE_SIZE,0);
}

int findqueueId(struct Message *message){
    int tempqueueId,j;
    j=FindUser(message);
    tempqueueId=Users[j].queueId;
    return tempqueueId;
}

void SendMessage(struct Message *message,int ServerKey){
    int i,j;
    struct Message returnmsg;
    strcpy(returnmsg.sender,(*message).sender);
    strcpy(returnmsg.reciever,(*message).reciever);
    (*message).type=MESSAGE_FROM;
    strcpy(returnmsg.message,(*message).message);
    returnmsg.type=SERVER_RESPONSE;
    int tempqueueId=findqueueId(message);
    for(i=0; i<NumberofUsers; i++){
        if(!strcmp(Users[i].name,(*message).reciever)){
            msgsnd(Users[i].queueId,message,MESSAGE_SIZE,0);
            strcpy(returnmsg.result,MESSAGE_RESULT_SUCCESS);
            msgsnd(tempqueueId,&returnmsg,MESSAGE_SIZE,0);
            printf("\n");
            return;
        }
    }
    if(i==NumberofUsers){
        strcpy(returnmsg.result, MESSAGE_RESULT_ERROR);
        strcpy(returnmsg.message, RESULT_NO_USER);
        msgsnd(tempqueueId,&returnmsg,MESSAGE_SIZE,0);
        printf("\n");
    }
}
void JoinGroup(struct Message *message){
    struct Message returnmsg;
    strcpy(returnmsg.sender,(*message).sender);
    strcpy(returnmsg.reciever,(*message).reciever);
    returnmsg.type=SERVER_RESPONSE;
    int i;
    int tempqueueId=findqueueId(message);
    for(i=0; i<NumberofGroups; i++){
        if(!strcmp(Groups[i].name,(*message).message)){
            int j;
            for(j=0; j<Groups[i].usersInGroup; j++)
                if(!strcmp(Groups[i].user[j].name,(*message).sender)){
                    strcpy(returnmsg.result,MESSAGE_RESULT_ERROR);
                    strcpy(returnmsg.message,RESULT_IN_GROUP);
                    msgsnd(tempqueueId,&returnmsg,MESSAGE_SIZE,0);
                    printf("%s juz jest w tej grupie\n",(*message).sender);
                    return;
                }
            if(j==Groups[i].usersInGroup){
                int x=FindUser(message);
                strcpy(Groups[i].user[Groups[i].usersInGroup].name,Users[x].name);
                Groups[i].usersInGroup++;
                strcpy(returnmsg.result,MESSAGE_RESULT_SUCCESS);
                msgsnd(tempqueueId,&returnmsg,MESSAGE_SIZE,0);
                printf("%s dolacza do grupy %s\n", (*message).sender, (*message).message);
            return;
            }
        }
    }
    if(i==NumberofGroups){
        strcpy(returnmsg.result,MESSAGE_RESULT_ERROR);
        strcpy(returnmsg.message,RESULT_NO_GROUP);
        msgsnd(tempqueueId,&returnmsg,MESSAGE_SIZE,0);
        printf("Nie ma takiej grupy %s\n", (*message).message);
    }

}
void ActiveUsers(struct Message *message){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    strcpy(returnmsg.message,"");
    int key=findqueueId(message);
    int i;
    for(i=0; i< NumberofUsers; i++){
        if(Users[i].logged){
            strcat(returnmsg.message,Users[i].name);
            strcat(returnmsg.message,"\n");
        }
    }
    msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
}

void GroupUsers(struct Message *message){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    strcpy(returnmsg.message,"");
    int key=findqueueId(message);
    int i;
    for(i=0; i< NumberofGroups; i++){
        if(!strcmp(Groups[i].name,(*message).message)){
            int j;
            printf("%s\n",Groups[i].name);
            for(j=0; j<Groups[i].usersInGroup;j++){
                strcat(returnmsg.message,Groups[i].user[j].name);
                strcat(returnmsg.message,"\n");
            }
            msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
            return;
        }
    }
}

void GroupList(struct Message *message){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    strcpy(returnmsg.message,"");
    int key=findqueueId(message);
    int i;
    for(i=0; i< NumberofGroups; i++){
        strcat(returnmsg.message,Groups[i].name);
        strcat(returnmsg.message,"\n");
    }
    msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
}

void MessageGroup(struct Message *message){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    int key=findqueueId(message);
    int i;
    for(i=0; i< NumberofGroups; i++){
        if(!strcmp(Groups[i].name,(*message).reciever)){
            int j;
            for(j=0; j<Groups[i].usersInGroup; j++){
                struct Message msg;
                strcpy(msg.message,(*message).message);
                strcpy(msg.reciever,Groups[i].user[j].name);
                strcpy(msg.sender,(*message).sender);
                int key2=findqueueId(&msg);
                SendMessage(&msg,key2);
            }
            strcpy(returnmsg.result,MESSAGE_RESULT_SUCCESS);
            msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
            printf("%s wysyla wiadomosc do grupy %s\n", (*message).sender,(*message).reciever);
            return;
        }
    }
    strcpy(returnmsg.result,MESSAGE_RESULT_ERROR);
    msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
}

void DeleteFromGroup(struct Message *message){
    struct Message returnmsg;
    returnmsg.type=SERVER_RESPONSE;
    int key=findqueueId(message);
    int i;
    for(i=0; i< NumberofGroups; i++){
        if(!strcmp(Groups[i].name,(*message).message)){
            int j;
            for(j=0; j<Groups[i].usersInGroup; j++){
                if(!strcmp(Groups[i].user[j].name,(*message).sender)){
                    strcpy(Groups[i].user[j].name,Groups[i].user[Groups[i].usersInGroup-1].name);
                    strcpy(Groups[i].user[Groups[i].usersInGroup-1].name," ");
                    Groups[i].usersInGroup--;
                    strcpy(returnmsg.result,MESSAGE_RESULT_SUCCESS);
                    msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
                    printf("%s usuwa sie z grupy grupy %s\n", (*message).sender,(*message).message);
                    return;
                }
            }
        }
    }
    strcpy(returnmsg.result,MESSAGE_RESULT_ERROR);
    msgsnd(key,&returnmsg,MESSAGE_SIZE,0);
}
int main()
{
    int ServerKey = msgget(serverPublicQueueKey,0644|IPC_CREAT);
    if(flag==0)
        msgctl(ServerKey,IPC_RMID,0);
    struct Message message;
    int rcv,rcv2;
    read_users();
    read_groups();
    while(true){
        rcv=msgrcv(ServerKey,&message,MESSAGE_SIZE,LOGGING_ATTEMPT_MESSAGE_TYPE,IPC_NOWAIT);
        if(rcv>0&&message.type==LOGGING_ATTEMPT_MESSAGE_TYPE){
            printf("%s probuje sie zalogowac\n", message.sender);
            Login(&message,ServerKey);
        }
        int i;
        for(i=0; i<NumberofUsers; i++){
            rcv2=msgrcv(Users[i].queueId, &message,MESSAGE_SIZE,0,IPC_NOWAIT);
            if(rcv2>0){
                if(message.type==LOGOUT_REQUEST){
                    printf("%s probuje sie wylogowac\n",message.sender);
                    Logout(message,Users[i].queueId);
                }
                else{
                    if(message.type==SEND_NORMAL_MESSAGE){
                        printf("%s wysyla wiadomosc do %s\n",message.sender,message.reciever);
                        SendMessage(&message,Users[i].queueId);
                    }
                    else{
                        if(message.type==JOIN_GROUP){
                            printf("%s chce dolaczyc do grup %s\n",message.sender,message.message);
                            JoinGroup(&message);
                        }
                        else{
                            if(message.type==LIST_USER){
                                printf("%s chce wyswietlic liste uzytkownikow\n", message.sender);
                                ActiveUsers(&message);
                            }
                            else{
                                if(message.type==LIST_GROUP_USERS){
                                    printf("%s chce wyswietlic uzytkownikow w grupie %s \n",message.sender,message.message);
                                    GroupUsers(&message);
                                }
                                else{
                                    if(message.type==LIST_GROUPS){
                                        printf("%s chce wyswietlic liste dostepnych grup \n", message.sender);
                                        GroupList(&message);
                                    }
                                    else{
                                        if(message.type==SEND_MESSAGE_TO_GROUP){
                                            printf("%s chce wyslac wiadomosc do grupy %s \n",message.sender,message.reciever);
                                            MessageGroup(&message);
                                        }
                                        else{
                                            if(message.type==LEAVE_GROUP){
                                                printf("%s chce opuscic grupe %s\n",message.sender,message.message);
                                                DeleteFromGroup(&message);
                                            }
                                        }
                                    }
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
