#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8081
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define HISTORY_SIZE 100

typedef struct {
    struct sockaddr_in address;
    int is_active;
    char username[50];
    char status[20];
} Client;

Client clients[MAX_CLIENTS];
char history[HISTORY_SIZE][BUFFER_SIZE];
int history_count = 0;
int server_socket;

/* ---------------- TIMESTAMP ---------------- */
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "[%H:%M:%S]", t);
}

/* ---------------- UTILS ---------------- */
void strip_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
        str[len - 1] = '\0';
}

void add_to_history(const char *msg) {
    if (history_count < HISTORY_SIZE) {
        strncpy(history[history_count++], msg, BUFFER_SIZE - 1);
        history[history_count - 1][BUFFER_SIZE - 1] = '\0';
    } else {
        for (int i = 1; i < HISTORY_SIZE; i++)
            strcpy(history[i - 1], history[i]);

        strncpy(history[HISTORY_SIZE - 1], msg, BUFFER_SIZE - 1);
        history[HISTORY_SIZE - 1][BUFFER_SIZE - 1] = '\0';
    }
}

int is_same_client(struct sockaddr_in *a, struct sockaddr_in *b) {
    return (a->sin_addr.s_addr == b->sin_addr.s_addr &&
            a->sin_port == b->sin_port);
}

int find_client(struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active &&
            is_same_client(&clients[i].address, addr)) {
            return i;
        }
    }
    return -1;
}

/* ---------------- BROADCAST ---------------- */
void broadcast(const char *msg) {
    add_to_history(msg);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active) {
            sendto(server_socket, msg, strlen(msg), 0,
                   (struct sockaddr *)&clients[i].address,
                   sizeof(clients[i].address));
        }
    }
}

/* ---------------- MAIN ---------------- */
int main() {
    struct sockaddr_in server_address, client_address;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_address);

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].is_active = 0;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server running on port %d...\n", PORT);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&client_address, &addr_len);

        if (bytes <= 0)
            continue;

        strip_newline(buffer);

        int idx = find_client(&client_address);

        /* -------- JOIN -------- */
        if (strncmp(buffer, "JOIN:", 5) == 0) {
            char username[50];
            sscanf(buffer, "JOIN:%49s", username);

            if (idx == -1) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].is_active) {
                        clients[i].address = client_address;
                        clients[i].is_active = 1;

                        strcpy(clients[i].username, username);
                        strcpy(clients[i].status, "online");

                        char timestamp[20], msg[150];
                        get_timestamp(timestamp, sizeof(timestamp));

                        snprintf(msg, sizeof(msg),
                                 "%s Notification: %s joined\n",
                                 timestamp, username);

                        broadcast(msg);
                        break;
                    }
                }
            }
        }

        /* -------- HELP (/help) -------- */
        else if (strcmp(buffer, "/help") == 0) {
            if (idx != -1) {
                char help_msg[] =
                    "\n--- UDP Commands ---\n"
                    "/help                Show commands\n"
                    "/status <state>      online/away/busy\n"
                    "/history             Show last messages\n"
                    "/exit                Leave chat\n"
                    "<text>               Broadcast message\n\n";

                sendto(server_socket, help_msg, strlen(help_msg), 0,
                       (struct sockaddr *)&client_address, addr_len);
            }
        }

        /* -------- HISTORY -------- */
        else if (strcmp(buffer, "/history") == 0) {
            if (idx != -1) {
                char msg[BUFFER_SIZE];

                strcpy(msg, "\n--- Chat History ---\n");

                for (int i = 0; i < history_count; i++) {
                    strncat(msg, history[i],
                            BUFFER_SIZE - strlen(msg) - 1);
                }

                strncat(msg, "\n", BUFFER_SIZE - strlen(msg) - 1);

                sendto(server_socket, msg, strlen(msg), 0,
                       (struct sockaddr *)&client_address, addr_len);
            }
        }

        /* -------- STATUS -------- */
        else if (strncmp(buffer, "STATUS:", 7) == 0) {
            if (idx != -1) {
                char new_status[20];
                sscanf(buffer, "STATUS:%19s", new_status);

                strcpy(clients[idx].status, new_status);

                char timestamp[20], msg[150];
                get_timestamp(timestamp, sizeof(timestamp));

                snprintf(msg, sizeof(msg), "%s %s is now %s\n",
                         timestamp, clients[idx].username, new_status);

                broadcast(msg);
            }
        }

        /* -------- LEAVE -------- */
        else if (strncmp(buffer, "LEAVE:", 6) == 0) {
            if (idx != -1) {
                char timestamp[20], msg[150];
                get_timestamp(timestamp, sizeof(timestamp));

                snprintf(msg, sizeof(msg), "%s %s left chat\n",
                         timestamp, clients[idx].username);

                clients[idx].is_active = 0;
                broadcast(msg);
            }
        }

        /* -------- MESSAGE -------- */
        else if (strncmp(buffer, "MSG:", 4) == 0) {
            if (idx != -1) {
                char text[BUFFER_SIZE];
                sscanf(buffer, "MSG:%4095[^\n]", text);

                char msg[BUFFER_SIZE];
                char timestamp[20];
                get_timestamp(timestamp, sizeof(timestamp));

                int used = snprintf(msg, BUFFER_SIZE, "%s %s [%s]: ",
                                    timestamp,
                                    clients[idx].username,
                                    clients[idx].status);

                int remaining = BUFFER_SIZE - used - 2;

                if (remaining > 0) {
                    strncat(msg, text, remaining);
                    strcat(msg, "\n");
                }

                broadcast(msg);
            }
        }

        else {
            printf("Unknown command: %s\n", buffer);
        }
    }

    close(server_socket);
    return 0;
}