#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>

enum { MAX_BUF = 4096 };

int create_server_socket(const char *port) {
    struct addrinfo hint = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
    };
    struct addrinfo *res = NULL;

    int gai_err = getaddrinfo(NULL, port, &hint, &res);
    if (gai_err) {
        return 0;
    }

    int sock = -1;
    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, 0);
        if (sock < 0) {
            continue;
        }

        int enable = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

        if (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            close(sock);
            sock = -1;
            continue;
        }
        if (listen(sock, SOMAXCONN) < 0) {
            close(sock);
            sock = -1;
            continue;
        }
        break;
    }

    return sock;
}

int main(int argc, char **argv) {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Something went wrong\n");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        fprintf(stderr, "Something went wrong\n");
        exit(EXIT_FAILURE);
    }


    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    const char *port = argv[1];

    int socket = create_server_socket(port);

    while (1) {
        struct sockaddr_in address;
        socklen_t address_len = sizeof(address);
        int connection = accept(socket, (struct sockaddr *)&address,&address_len);

        int ret = fork();
        if (ret == 0) {
            char buf[MAX_BUF];
            read(connection, buf, sizeof(buf));
            dup2(connection, STDIN_FILENO);
            dup2(connection, STDOUT_FILENO);

            char *arguments[MAX_BUF] = {NULL};
            const char *delim = " \t\n\r\v";

            char *token = strtok(buf, delim);
            size_t cur = 0;
            while (token != NULL) {
                arguments[cur++] = token;
                token = strtok(NULL, delim);
            }
            execvp(arguments[0], (char *const *)arguments);

            _exit(0);
        } else if (ret < 0) {
            close(connection);
            return 0;
        } else {
            close(connection);
        }
    }
}
