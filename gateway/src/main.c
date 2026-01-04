#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "gateway_core.h"
#include "node_manager.h"

void handle_sigint(int sig) {
    (void)sig; // 告诉编译器,避免warning
    printf("\nShutting down gateway...\n");
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    GatewayServer server;
    if (init_gateway(&server) != 0) {
        fprintf(stderr, "Failed to initialize gateway\n");
        return EXIT_FAILURE;
    }

    // 待办：在此初始化线程池和互斥锁
    init_node_manager();
    run_gateway_loop(&server);

    return 0;
}