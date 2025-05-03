#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 8081

char* parse(char line[], const char symbol[]);
char* parse_method(char line[], const char symbol[]);
char* find_token(char line[], const char symbol[], const char match[]);
int send_message(int fd, char image_path[], char head[]);

char http_header[25] = "HTTP/1.1 200 OK\r\n";

int main() {
    int server_fd, new_socket, pid;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    memset(address.sin_zero, '\0', sizeof address.sin_zero);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == 0) {
            char buffer[30000] = {0};
            read(new_socket, buffer, 30000);
            printf("Request:\n%s\n", buffer);

            char *method = parse_method(buffer, " ");
            char *path = parse(buffer, " ");

            char *copy = malloc(strlen(path) + 1);
            strcpy(copy, path);
            char *ext = parse(copy, ".");

            char *response = malloc(1024);
            strcpy(response, http_header);

            if (strncmp(method, "GET", 3) == 0) {
                if (strlen(path) <= 1) {
                    strcat(response, "Content-Type: text/html\r\n\r\n");
                    send_message(new_socket, "./index.html", response);
                } else {
                    char full_path[512] = ".";
                    strcat(full_path, path);

                    if (strcmp(ext, "css") == 0)
                        strcat(response, "Content-Type: text/css\r\n\r\n");
                    else if (strcmp(ext, "js") == 0)
                        strcat(response, "Content-Type: text/javascript\r\n\r\n");
                    else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
                        strcat(response, "Content-Type: image/jpeg\r\n\r\n");
                    else
                        strcat(response, "Content-Type: text/plain\r\n\r\n");

                    send_message(new_socket, full_path, response);
                }
            }

            close(new_socket);
            free(copy);
            free(response);
            exit(0);
        } else {
            close(new_socket);
        }
    }

    close(server_fd);
    return 0;
}

int send_message(int fd, char image_path[], char head[]) {
    struct stat stat_buf;
    write(fd, head, strlen(head));
    int fdimg = open(image_path, O_RDONLY);
    if (fdimg < 0) {
        perror("Cannot open file");
        return -1;
    }

    fstat(fdimg, &stat_buf);
    int total = stat_buf.st_size;
    int block = stat_buf.st_blksize;
    while (total > 0) {
        int sent = sendfile(fd, fdimg, NULL, block);
        if (sent < 0) break;
        total -= sent;
    }
    close(fdimg);
    return 0;
}

char* parse(char line[], const char symbol[]) {
    char *copy = malloc(strlen(line) + 1);
    strcpy(copy, line);
    strtok(copy, symbol);
    char *token = strtok(NULL, " ");
    return token ? token : "";
}

char* parse_method(char line[], const char symbol[]) {
    char *copy = malloc(strlen(line) + 1);
    strcpy(copy, line);
    char *token = strtok(copy, symbol);
    return token ? token : "";
}

char* find_token(char line[], const char symbol[], const char match[]) {
    char *copy = malloc(strlen(line) + 1);
    strcpy(copy, line);
    char *token = strtok(copy, symbol);
    while (token) {
        if (strncmp(token, match, strlen(match)) == 0)
            return token;
        token = strtok(NULL, symbol);
    }
    return "";
}
