#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h> 

#define VERSION "3.1-stable-daemon"
#define MAX_EVENTS 10000
#define BUFFER_SIZE 8192
#define MAX_HOST_LEN 256 

int listen_port = 0;
char remote_host[MAX_HOST_LEN] = {0};
int remote_port = 0;
int epoll_fd;

struct conn_data {
    int fd;
    char buffer[BUFFER_SIZE];
    size_t buf_len;
};

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 核心处理逻辑
int handle_handshake_and_socat(struct conn_data *conn) {
    // 使用 MSG_PEEK 查看数据但不从内核缓冲区移除，解决残留风险
    ssize_t n = recv(conn->fd, conn->buffer, BUFFER_SIZE - 1, MSG_PEEK);
    
    if (n < 0) {
        if (errno != EWOULDBLOCK) return -1;
        return 0;
    }
    if (n == 0) return -1;

    conn->buffer[n] = '\0';

    // 匹配客户端伪装头
    if (strncasecmp(conn->buffer, "CONNECT", 7) == 0 || 
        strncasecmp(conn->buffer, "GET", 3) == 0 || 
        strncasecmp(conn->buffer, "POST", 4) == 0) {
        
        // 1. 先把 PEEK 出来的数据真正读走（清空 HTTP 头）
        char junk[BUFFER_SIZE];
        read(conn->fd, junk, n); 

        // 2. 回复 200 OK 告知客户端代理已就绪
        const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        write(conn->fd, response, strlen(response));
        
        char socat_target[MAX_HOST_LEN + 32];
        snprintf(socat_target, sizeof(socat_target), "TCP:%s:%d", remote_host, remote_port);

        pid_t pid = fork();
        if (pid < 0) return -1;
        
        if (pid == 0) { // 子进程执行 socat
            dup2(conn->fd, STDIN_FILENO);
            dup2(conn->fd, STDOUT_FILENO);
            dup2(conn->fd, STDERR_FILENO);
            if (conn->fd > STDERR_FILENO) close(conn->fd);
            
            execlp("socat", "socat", "-", socat_target, (char *)NULL);
            exit(1);
        }
        
        return -1; // 父进程移除该 FD
    }

    if (n >= BUFFER_SIZE - 1) return -1; 
    return 0;
}

void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    int do_daemon = 0;
    int opt_cmd;

    // 解析参数，增加 -d 支持
    while ((opt_cmd = getopt(argc, argv, "l:r:d")) != -1) {
        switch (opt_cmd) {
            case 'l': listen_port = atoi(optarg); break;
            case 'r': {
                char *colon = strrchr(optarg, ':');
                if (colon) {
                    *colon = '\0';
                    strncpy(remote_host, optarg, MAX_HOST_LEN-1);
                    remote_port = atoi(colon + 1);
                }
                break;
            }
            case 'd': do_daemon = 1; break;
        }
    }

    if (listen_port == 0 || remote_port == 0) {
        fprintf(stderr, "Usage: %s -l <port> -r <host:port> [-d]\n", argv[0]);
        exit(1);
    }

    // 后台运行逻辑
    if (do_daemon) {
        if (daemon(1, 0) < 0) {
            perror("daemon failed");
            return 1;
        }
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld_handler); 

    int listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
    int no = 0;
    setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in6 server_addr = {0};
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(listen_port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ||
        listen(listen_fd, SOMAXCONN) < 0) {
        perror("Bind/Listen failed");
        return 1;
    }
    set_nonblocking(listen_fd);

    epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_fd) {
                int client_fd = accept(listen_fd, NULL, NULL);
                if (client_fd > 0) {
                    set_nonblocking(client_fd);
                    struct conn_data *conn = calloc(1, sizeof(struct conn_data));
                    conn->fd = client_fd;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.ptr = conn;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                }
            } else {
                struct conn_data *conn = events[i].data.ptr;
                if (handle_handshake_and_socat(conn) == -1) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
                    close(conn->fd);
                    free(conn);
                }
            }
        }
    }
    return 0;
}

