#ifndef GATEWAY_CORE_H
#define GATEWAY_CORE_H

#include <netinet/in.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define UDP_BUFFER_SIZE 1024
#define LISTEN_PORT 8888  // 监听心跳广播端口

typedef struct {
    int udp_fd;
    int timer_fd;
    int epoll_fd;
    struct sockaddr_in server_addr;
} GatewayServer;

// 初始化服务器硬件资源 (Socket, Timerfd, Epoll)
int init_gateway(GatewayServer *server);

// 进入 epoll 核心事件循环
void run_gateway_loop(GatewayServer *server);

// 设置文件描述符为非阻塞模式
int set_nonblocking(int fd);

#endif