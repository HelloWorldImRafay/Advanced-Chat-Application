
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 2048

/* ---------------- BASE64 ---------------- */
void encode_base64(const unsigned char *data, size_t input_length, char *encoded_data) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (input_length--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) *encoded_data++ = base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i > 0) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) *encoded_data++ = base64_chars[char_array_4[j]];
        while (i++ < 3) *encoded_data++ = '=';
    }

    *encoded_data = '\0';
}

/* ---------------- MAIN ---------------- */
int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf(" Connected to TCP Server!\n");
    printf(" Type /login <username> <password> to begin\n");
    printf(" Type /help after login to see commands\n\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        select(sock + 1, &readfds, NULL, NULL, NULL);

        /* RECEIVE FROM SERVER */
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(sock, buffer, BUFFER_SIZE - 1);

            if (valread == 0) {
                printf("❌ Server disconnected.\n");
                break;
            }

            printf("%s", buffer);
        }

        /* USER INPUT */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);

            /* FILE COMMAND */
            if (strncmp(buffer, "/file", 5) == 0) {
                char command[20], user[50], filepath[100];

                if (sscanf(buffer, "%s %s %s", command, user, filepath) == 3) {

                    FILE *file = fopen(filepath, "rb");
                    if (!file) {
                        printf("❌ Cannot open file\n");
                        continue;
                    }

                    unsigned char file_data[500];
                    int bytes = fread(file_data, 1, sizeof(file_data), file);
                    fclose(file);

                    char encoded[1024];
                    encode_base64(file_data, bytes, encoded);

                    char msg[BUFFER_SIZE];
                    sprintf(msg, "/file %s %s\n", user, encoded);

                    send(sock, msg, strlen(msg), 0);
                    printf("✅ File sent to %s\n", user);

                } else {
                    printf("Usage: /file <user> <filepath>\n");
                }
            }

            /* NORMAL COMMANDS */
            else {
                send(sock, buffer, strlen(buffer), 0);

                if (strncmp(buffer, "/exit", 5) == 0) {
                    printf("Disconnected.\n");
                    break;
                }
            }
        }
    }

    close(sock);
    return 0;
}
