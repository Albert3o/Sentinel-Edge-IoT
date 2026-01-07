#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syslog.h>
#include <string.h>
#include "gateway_core.h"
#include "node_manager.h"
#include "worker_pool.h"
#include "cloud_client.h"
#include "utils.h"


GatewayServer server;
int is_daemon=0;
pid_t global_worker_pid = -1;
/*
为什么要用volatile：
在守护进程中，is_shutdown 是在主循环里读取的，但它是在 signal_handler（信号处理函数）里修改的。
信号处理函数是异步的：它由内核触发，主循环根本不知道什么时候发生了修改。
关键点：如果不加 volatile，主循环的 while 或者 if(is_shutdown) 可能一直使用寄存器里的旧值 0，导致即使你发了 kill 信号，守护进程也无法退出。
sig_atomic_t 又是什么？
sig_atomic_t 能保证对这个变量的 读/写操作是原子的（不会被中断）。
在 32/64 位系统上，修改一个整数可能需要多个指令，如果在修改一半时发生了信号，值就会错乱。sig_atomic_t 避免了这个问题。
*/
volatile sig_atomic_t is_shutdown = 0;


// 子进程的清理函数（替代 Lambda）
void worker_cleanup_handler(int sig) {
    syslog(LOG_INFO, "子进程捕获 SIGTERM，释放资源中...");
    worker_pool_destroy();
    cloud_client_cleanup();
    exit(0);
}


void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGHUP:
        //syslog(LOG_WARNING, "收到SIGHUP信号...");
        break;
    case SIGTERM:
    case SIGINT:
        //syslog(LOG_NOTICE, "接收到终止信号，准备退出守护进程...");
        //syslog(LOG_NOTICE, "向子进程发送SIGTERM信号...");
        is_shutdown = 1;
        if (global_worker_pid > 0) {
                kill(global_worker_pid, SIGTERM);
            }
        break;
    default:
        //syslog(LOG_INFO, "收到未处理信号: %d", sig);
    }
}

void my_daemonize()
{
    pid_t pid;
    // Fork off the parent process
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);
    // 信号处理
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 重置umask-0777
    umask(0);
    // 将工作目录切换为根目录
    chdir("/");
    // 关闭所有打开的文件描述符
    for (int x = 0; x <= sysconf(_SC_OPEN_MAX); x++)
    {
        close(x);
    }
    openlog("sentinel_gateway", LOG_PID, LOG_DAEMON);
}


// 子进程（真正干活的网关）的逻辑封装
int run_worker_logic() {
    // 子进程自己的信号处理
    signal(SIGTERM, worker_cleanup_handler);

    init_node_manager();
    cloud_client_init();
    
    if (worker_pool_init(4) != 0) {
        log_error("[Monitor] 线程池初始化失败");
        cloud_client_cleanup();
        return -1;
    }

    if (init_gateway(&server) != 0) {
        log_error("[Monitor] 网关核心初始化失败");
        return -1;
    }

    log_message(LOG_INFO, "[Monitor] 网关子进程逻辑启动成功");
    run_gateway_loop(&server);
    return 0;
}


int main(int argc, char *argv[]) {
    if(argc == 2 && strcmp(argv[1], "-d") == 0){
        is_daemon=1;
        my_daemonize();
    }

    while (!is_shutdown) {
        global_worker_pid = fork();

        if (global_worker_pid > 0) {
            // 守护进程逻辑
            log_message(LOG_INFO, "[Monitor] 监控中：子进程 PID = %d", global_worker_pid);
            
            int status;
            waitpid(global_worker_pid, &status, 0);

            if (is_shutdown) {
                log_message(LOG_INFO, "[Monitor] 收到关闭指令，不再重启子进程");
                break;
            }

            // 检查子进程是否是异常退出的
            if (WIFEXITED(status)) { // Wait If Exited. 如果子进程是“自愿退出”的，返回真。
                log_message(LOG_INFO, "[Monitor] 子进程正常退出(退出码:%d),3秒后重启...", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) { // Wait If Signaled. 如果子进程是被信号终止的，返回真
                log_message(LOG_INFO, "[Monitor] 子进程被信号 %d 杀死,3秒后重启...", WTERMSIG(status));
            }
            sleep(3);
        }else if (global_worker_pid == 0) {
            // 子进程逻辑
            if(run_worker_logic() != 0) {
                log_error("[Monitor] 子进程运行出错，退出");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }else {
            log_error("[Monitor] fork failed");
            sleep(10);
        }
    }

    closelog();
    return EXIT_SUCCESS;
}
