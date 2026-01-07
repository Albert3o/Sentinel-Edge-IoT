#ifndef CLOUD_CLIENT_H
#define CLOUD_CLIENT_H

#include <stdint.h>

// Firebase 配置
#define FIREBASE_URL "https://sentineledgeiot-default-rtdb.firebaseio.com/alerts.json?auth=9lAkjK9moYXwj11RMuXwm1Ht6tZO6YeA87F6o7Uw"
// 初始化 Cloud Client (如 curl 环境)
void cloud_client_init();

// 清理环境
void cloud_client_cleanup();

// 执行上传任务 (将被线程池调用)
void upload_alert_task(void *arg);

// 报警任务参数结构体
typedef struct {
    uint32_t node_id;
    uint16_t ldr_value;
    uint8_t  severity;
} AlertTaskArgs;

#endif