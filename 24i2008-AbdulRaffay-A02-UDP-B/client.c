#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8081
#define BUFFER_SIZE 2048

int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    char username[50];

    fd_set readfds;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* -------- USERNAME INPUT -------- */
    printf("JOIN: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    /* -------- SEND JOIN -------- */
    char join_msg[100];
    snprintf(join_msg, sizeof(join_msg), "JOIN:%s", username);

    sendto(sock, join_msg, strlen(join_msg), 0,
           (struct sockaddr *)&server_address,
           sizeof(server_address));

    printf("✅ Connected to UDP server\n");
    printf("Commands:\n");
    printf("  /status <online/away/busy>\n");
    printf("  /help\n");
    printf("  /history\n");
    printf("  /exit\n\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        select(sock + 1, &readfds, NULL, NULL, NULL);

        /* -------- RECEIVE -------- */
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);

            int valread = recvfrom(sock, buffer,
                                   BUFFER_SIZE - 1, 0,
                                   NULL, NULL);

            if (valread > 0) {
                printf("%s", buffer);
            }
        }

        /* -------- USER INPUT -------- */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);

            buffer[strcspn(buffer, "\n")] = 0;

            /* -------- EXIT -------- */
            if (strcmp(buffer, "/exit") == 0) {
                char leave_msg[100];
                snprintf(leave_msg, sizeof(leave_msg),
                         "LEAVE:%s", username);

                sendto(sock, leave_msg, strlen(leave_msg), 0,
                       (struct sockaddr *)&server_address,
                       sizeof(server_address));

                printf("Disconnected from server.\n");
                break;
            }

            /* -------- HELP -------- */
            else if (strcmp(buffer, "/help") == 0) {
                sendto(sock, "/help", 5, 0,
                       (struct sockaddr *)&server_address,
                       sizeof(server_address));
            }

            /* -------- HISTORY -------- */
            else if (strcmp(buffer, "/history") == 0) {
                sendto(sock, "/history", 8, 0,
                       (struct sockaddr *)&server_address,
                       sizeof(server_address));
            }

            /* -------- STATUS -------- */
            else if (strncmp(buffer, "/status", 7) == 0) {
                char status[20] = "online";

                sscanf(buffer, "%*s %19s", status);

                char msg[100];
                snprintf(msg, sizeof(msg),
                         "STATUS:%s", status);

                sendto(sock, msg, strlen(msg), 0,
                       (struct sockaddr *)&server_address,
                       sizeof(server_address));
            }

            /* -------- NORMAL MESSAGE -------- */
            else {
                char msg[BUFFER_SIZE];

                int max_len = BUFFER_SIZE - 5;
                snprintf(msg, sizeof(msg),
                         "MSG:%.*s", max_len, buffer);

                sendto(sock, msg, strlen(msg), 0,
                       (struct sockaddr *)&server_address,
                       sizeof(server_address));
            }
        }
    }

    close(sock);
    return 0;
}