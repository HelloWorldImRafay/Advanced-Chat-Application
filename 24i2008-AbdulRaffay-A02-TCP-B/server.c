#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define HISTORY_SIZE 100

typedef struct {
    int socket;
    char username[50];
    int authenticated;
    char status[20];
} Client;

Client clients[MAX_CLIENTS];
char history[HISTORY_SIZE][BUFFER_SIZE];
int history_count = 0;

/*---------------- TIME ----------------*/
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "[%H:%M:%S]", t);
}

/*---------------- AUTH ----------------*/
int check_credentials(char *user, char *pass) {
    return (strcmp(user,"rafay")==0 && strcmp(pass,"12345678")==0) ||
           (strcmp(user,"subhan")==0 && strcmp(pass,"12345678")==0) ||
           (strcmp(user,"sameer")==0 && strcmp(pass,"123345678")==0);
}

/*---------------- UTILS ---------------- */
void strip_newline(char *str) {
    int len = strlen(str);
    if (len>0 && str[len-1]=='\n') str[len-1]='\0';
}

void add_to_history(const char *msg) {
    if (history_count < HISTORY_SIZE) {
        strncpy(history[history_count++], msg, BUFFER_SIZE-1);
    } else {
        for (int i=1;i<HISTORY_SIZE;i++)
            strncpy(history[i-1], history[i], BUFFER_SIZE-1);
        strncpy(history[HISTORY_SIZE-1], msg, BUFFER_SIZE-1);
    }
}

int find_user_socket(const char *username) {
    for (int i=0;i<MAX_CLIENTS;i++) {
        if (clients[i].socket>0 &&
            clients[i].authenticated &&
            strcmp(clients[i].username,username)==0)
            return clients[i].socket;
    }
    return -1;
}

void broadcast_message(const char *msg, int sender) {
    add_to_history(msg);
    for (int i=0;i<MAX_CLIENTS;i++) {
        if (clients[i].socket>0 &&
            clients[i].authenticated &&
            clients[i].socket!=sender) {
            send(clients[i].socket,msg,strlen(msg),0);
        }
    }
}

/* ---------------- MAIN ---------------- */
int main() {
    int server_socket,new_socket,max_sd,sd;
    struct sockaddr_in server_address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    for(int i=0;i<MAX_CLIENTS;i++){
        clients[i].socket=0;
        clients[i].authenticated=0;
    }

    server_socket=socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    server_address.sin_family=AF_INET;
    server_address.sin_addr.s_addr=INADDR_ANY;
    server_address.sin_port=htons(PORT);

    bind(server_socket,(struct sockaddr*)&server_address,sizeof(server_address));
    listen(server_socket,5);

    printf("TCP Server running on port %d...\n",PORT);

    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_socket,&readfds);
        max_sd=server_socket;

        for(int i=0;i<MAX_CLIENTS;i++){
            sd=clients[i].socket;
            if(sd>0) FD_SET(sd,&readfds);
            if(sd>max_sd) max_sd=sd;
        }

        select(max_sd+1,&readfds,NULL,NULL,NULL);

        /* NEW CONNECTION */
        if(FD_ISSET(server_socket,&readfds)){
            struct sockaddr_in client_address;
            socklen_t addrlen=sizeof(client_address);

            new_socket=accept(server_socket,(struct sockaddr*)&client_address,&addrlen);

            char *welcome="Welcome!\nUse: /login <username> <password>\n";
            send(new_socket,welcome,strlen(welcome),0);

            for(int i=0;i<MAX_CLIENTS;i++){
                if(clients[i].socket==0){
                    clients[i].socket=new_socket;
                    strcpy(clients[i].status,"online");
                    break;
                }
            }
        }

        /* HANDLE CLIENTS */
        for(int i=0;i<MAX_CLIENTS;i++){
            sd=clients[i].socket;

            if(FD_ISSET(sd,&readfds)){
                memset(buffer,0,BUFFER_SIZE);
                int valread = recv(sd, buffer, BUFFER_SIZE-1, 0);

                if(valread<=0){
                    if(clients[i].authenticated){
                        char timebuf[20];
                        get_timestamp(timebuf,sizeof(timebuf));

                        char msg[100];
                        snprintf(msg,sizeof(msg),
                                 "%s Notification: %s left chat\n",
                                 timebuf, clients[i].username);
                        broadcast_message(msg,sd);
                    }
                    close(sd);
                    clients[i].socket=0;
                    clients[i].authenticated=0;
                }

                else{
                    strip_newline(buffer);

                    char command[50]={0},arg1[50]={0},arg2[1024]={0};
                    sscanf(buffer,"%49s %49s %1023[^\n]",command,arg1,arg2);

                    if(strcmp(command,"/login")==0){
                        if(check_credentials(arg1,arg2)){
                            strcpy(clients[i].username,arg1);
                            clients[i].authenticated=1;

                            char msg[100];
                            snprintf(msg,sizeof(msg),
                                     "Login successful! Welcome %s\n",arg1);
                            send(sd,msg,strlen(msg),0);

                            char timebuf[20];
                            get_timestamp(timebuf,sizeof(timebuf));

                            snprintf(msg,sizeof(msg),
                                     "%s Notification: %s joined\n",
                                     timebuf,arg1);
                            broadcast_message(msg,sd);
                        } else {
                            send(sd,"Invalid credentials\n",21,0);
                        }
                    }

                    else if(!clients[i].authenticated){
                        send(sd,"Login first\n",12,0);
                    }

                    else if(strcmp(command,"/help")==0){
                        char *help =
                        "Commands:\n"
                        "/msg <user> <text>\n"
                        "/file <user> <path>\n"
                        "/status <online/away/busy>\n"
                        "/history \n"
                        "/help\n"
                        "/exit\n";

                        send(sd, help, strlen(help), 0);
                    }

                    else if(strcmp(command,"/msg")==0){

                        
                        if(strcmp(clients[i].username, arg1) == 0){
                            send(sd, "You cannot message yourself\n", 30, 0);
                        }

                        else{
                            int target = find_user_socket(arg1);

                            if(target == -1){
                                send(sd, "User not found\n", 15, 0);
                            }

                            else{
                                char msg[BUFFER_SIZE];
                                snprintf(msg, sizeof(msg),
                                        "[Private %s]: %.900s\n",
                                        clients[i].username, arg2);

                                add_to_history(msg);      // store in history
                                send(target, msg, strlen(msg), 0);
                            }
                        }
                    }

                    else if(strcmp(command,"/file")==0){
                        int target=find_user_socket(arg1);
                        if(target!=-1){
                            char msg[BUFFER_SIZE];
                            snprintf(msg,sizeof(msg),
                                     "[File from %s]: %.900s\n",
                                     clients[i].username,arg2);

                            add_to_history(msg);
                            send(target,msg,strlen(msg),0);
                        }
                    }

                    else if(strcmp(command,"/status")==0){
                        strcpy(clients[i].status,arg1);

                        char msg[100];
                        snprintf(msg,sizeof(msg),
                                 "%.40s is now %.40s\n",
                                 clients[i].username,arg1);

                        add_to_history(msg);
                        send(sd,msg,strlen(msg),0);
                        broadcast_message(msg,sd);
                    }

                    else if(strcmp(command,"/history")==0){
                        int num = atoi(arg1);
                        if(num<=0 || num>history_count) num = history_count;

                        int start = history_count - num;
                        if(start<0) start=0;

                        for(int h=start;h<history_count;h++)
                            send(sd,history[h],strlen(history[h]),0);
                    }

                    else if(strcmp(command,"/exit")==0){
                        char timebuf[20];
                        get_timestamp(timebuf,sizeof(timebuf));

                        char msg[100];
                        snprintf(msg,sizeof(msg),
                                 "%s %s left chat\n",
                                 timebuf, clients[i].username);
                        broadcast_message(msg,sd);

                        close(sd);
                        clients[i].socket=0;
                        clients[i].authenticated=0;
                    }

                    else{
                        char msg[BUFFER_SIZE];
                        snprintf(msg,sizeof(msg),
                                 "%s [%s]: %.900s\n",
                                 clients[i].username,
                                 clients[i].status,
                                 buffer);
                        broadcast_message(msg,sd);
                    }
                }
            }
        }
    }
}