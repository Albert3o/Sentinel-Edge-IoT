#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>

extern int is_daemon;

// 通用日志打印
void log_message(int priority, const char *format, ...);

// 专门处理 errno 的错误打印
void log_error(const char *msg);

#endif