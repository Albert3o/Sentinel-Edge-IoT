#define _POSIX_C_SOURCE 199309L  // 开启 POSIX 标准支持（包含实时扩展） --> 识别CLOCK_MONOTONIC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <time.h>

#include "gateway_core.h"
#include "protocol.h"
#include "utils.h"
#include "node_manager.h"


int ser_nonblocking(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1){
        exit(EXIT_FAILURE);
    }
    return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

int init_gateway(GatewayServer *server) {
    // 1. 创建 UDP Socket
    server->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server->udp_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* 
     * 设置地址复用
     * 作用：允许服务器在关闭后立即重新启动，而不会因为“端口被占用”而报错。
     */
    int opt = 1;
    setsockopt(server->udp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server->server_addr, 0, sizeof(server->server_addr));
    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port = htons(LISTEN_PORT);

    if (bind(server->udp_fd, (struct sockaddr *)&server->server_addr, sizeof(server->server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server->udp_fd);

    // 2. 创建 Timerfd (每1秒触发一次)
    server->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    struct itimerspec new_value;
    new_value.it_interval.tv_sec = 1;  // 周期 1s
    new_value.it_interval.tv_nsec = 0;
    new_value.it_value.tv_sec = 1;     // 首次触发时间
    new_value.it_value.tv_nsec = 0;
    timerfd_settime(server->timer_fd, 0, &new_value, NULL);

    // 3. 创建并初始化 Epoll
    server->epoll_fd = epoll_create1(0);
    struct epoll_event ev;

    // 注册 UDP Socket
    ev.events = EPOLLIN; // 水平触发即可，UDP不会像TCP那样积压大量字节流
    ev.data.fd = server->udp_fd;
    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->udp_fd, &ev);

    // 注册 Timerfd
    ev.events = EPOLLIN;
    ev.data.fd = server->timer_fd;
    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->timer_fd, &ev);
    /*
    上述的ev是传入指针给内核，内核会进行值拷贝，将其存到内核空间，然后内核会将其挂到epoll的红黑树上
    因此ev是局部变量不影响后续拿到对应的epoll_event值
    

    如何拿回存在epoll红黑树中的数据：
    struct epoll_event events[10]; // 准备一个数组，用来接收“发生”的事件
    while (1) {
        // n 是本次触发事件的数量
        int n = epoll_wait(server->epoll_fd, events, 10, -1); 

        for (int i = 0; i < n; i++) {
            // 关键点：events[i].data 就是你当初通过 epoll_ctl 存进去的那份拷贝！ events[i]就是上面的ev
            if (events[i].data.fd == server->udp_fd) {
                handle_udp_msg(server->udp_fd);
            } else if (events[i].data.fd == server->timer_fd) {
                handle_timer_exp(server->timer_fd);
            }
        }
    }
    */
    return 0;
}

void handle_udp_packet(int fd) {
    IoTProtocolPacket packet;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    ssize_t len = recvfrom(fd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len);
    
    if (len == sizeof(IoTProtocolPacket) && packet.version == FIRST_VERSION) {
        // 调用 node_manager 更新内存表
        update_node(packet.node_id, packet.ldr_value, packet.pir_state, (NodeRole)packet.node_role);
        
        // 如果是异常包(PKT_ALERT)，可以在这里触发云端上传任务逻辑
        if (packet.pkt_type == PKT_ALERT) {
            printf("[ALERT] High severity event from Master 0x%X!\n", packet.node_id);
        }
    }
}

void handle_timer_tick(int fd) {
    uint64_t expirations;
    read(fd, &expirations, sizeof(expirations)); // 必须读取，否则 epoll 会持续触发
    
    // 检查 5 秒未报到的节点（PRD 需求）
    check_node_timeouts(5);
    
    // 每 5 秒打印一次节点表（可选，调试用）
    static int counter = 0;
    if (++counter % 5 == 0) {
        dump_node_table();
    }
}

void run_gateway_loop(GatewayServer *server) {
    struct epoll_event events[MAX_EVENTS];
    printf("Gateway Gateway listening on port %d...\n", LISTEN_PORT);

    while (1) {
        int nfds = epoll_wait(server->epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server->udp_fd) {
                handle_udp_packet(server->udp_fd);
            } else if (events[i].data.fd == server->timer_fd) {
                handle_timer_tick(server->timer_fd);
            }
        }
    }
}