#include "utils.h"


void log_message(int priority, const char *format, ...) {
    va_list args;
    va_start(args, format);

    if (is_daemon) {
        // 守护进程模式：写入系统日志 (/var/log/syslog)
        vsyslog(priority, format, args);
    } else {
        // 前台模式：打印到终端
        printf("[%d] ", priority); // 将 priority 转换为简单的字符串前缀（可选）
        vprintf(format, args);
        printf("\n");
    }

    va_end(args);
}


void log_error(const char *msg) {
    // 获取当前系统的错误描述信息 (相当于 perror 内部做的操作)
    char *err_str = strerror(errno);

    if (is_daemon) {
        // 守护进程模式：记录到 syslog
        // %s: %s 格式化为：用户自定义信息: 系统错误原因
        syslog(LOG_ERR, "%s: %s", msg, err_str);
    } else {
        // 前台模式：打印到 stderr (标准错误流)
        fprintf(stderr, "[ERROR] %s: %s\n", msg, err_str);
    }
}
