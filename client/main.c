#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

enum { MAX_BUF = 4096 };

int get_socket(char *host, char *service) {
    struct addrinfo hint = {
        .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM, .ai_protocol = 0};

    struct addrinfo *res = NULL;
    int gai_err;

    gai_err = getaddrinfo(host, service, &hint, &res);
    if (gai_err) {
        return 0;
    }

    int sock = -1;
    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, 0);
        if (sock < 0) {
            continue;
        }
        if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            close(sock);
            sock = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    return sock;
}

int main(int argc, char **argv) {
    char *host = argv[1];
    char *service = argv[2];

    if (strcmp(argv[3], "spawn") == 0) {
        char buf[MAX_BUF] = {0};
        size_t sum_len = 0;

        for (int i = 4; i < argc; ++i) {
            size_t cur_len = strlen(argv[i]);
            if (sum_len + cur_len + 1 > MAX_BUF) {
                fprintf(stderr, "Too long command\n");
                return 1;
            }
            snprintf(buf + sum_len, MAX_BUF - sum_len - 1, "%s ", argv[i]);
            sum_len += strlen(argv[i]) + 1;
        }

        int sock = get_socket(host, service);

        if (sock < 0) {
            fprintf(stderr, "Connection refused\n");
            return 1;
        }

        int epoll_fd = epoll_create1(0);
        if (epoll_fd < 0) {
            perror("epoll");
            return 1;
        }

        struct epoll_event evt = {EPOLLIN, {.fd = 0}};
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &evt);
        evt.events = EPOLLOUT;
        evt.data.fd = 1;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &evt);

        int send = 0;
        while (1) {
            int timeout = 10;
            evt.data.fd = 1;

            epoll_wait(epoll_fd, &evt, 1, timeout);

            char local_buf[MAX_BUF];
            if (evt.data.fd == 0) {
                ssize_t ret = read(sock, local_buf, sizeof(local_buf) - 1);
                write(STDOUT_FILENO, local_buf, ret);
            } else {
                if (send == 0) {
                    write(sock, buf, strlen(buf));
                    send = 1;
                } else {
                    ssize_t ret = read(STDIN_FILENO, local_buf, sizeof(local_buf) - 1);
                    write(sock, local_buf, ret);
                }
            }
        }
    }
}
