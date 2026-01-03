#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "gateway_core.h"

void handle_sigint(int sig) {
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

    run_gateway_loop(&server);

    return 0;
}