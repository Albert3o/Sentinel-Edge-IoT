#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "protocol.h"
/*
node_manager采用单链表的结构来管理ESP结点

为什么要用单链表（Linked List）？
在工业级边缘侧开发中，选择链表而不是数组有几个考量：
动态伸缩：你事先不知道会有 3 个还是 30 个 ESP32 节点。链表可以按需分配。
删除效率：当某个节点（比如在链表中间）掉线时，链表删除节点只需要改变指针指向，不需要像数组那样移动大量内存。
*/

// 节点状态结构体
typedef struct Node {
    uint32_t node_id;
    uint16_t last_ldr;      // 最新状态：我那里的光照强度是多少？ 
    uint8_t  last_pir;      // 最新状态：我那里现在有人在动吗？
    NodeRole role;
    time_t last_alert_time; // 用于报警限流
    time_t last_seen;     // 记录最后一次收到心跳的系统时间,用于心跳/活跃检查
    struct Node *next;      // 单链表结构
} Node;

// 初始化节点表
void init_node_manager();

// 更新或添加节点信息（线程安全）
void update_node(uint32_t id, uint16_t ldr, uint8_t pir, NodeRole role);

// 检查超时节点并清理（线程安全）
void check_node_timeouts(int timeout_sec);

// 打印当前所有在线节点（调试用）
void dump_node_table();

// 检查并更新上传时间戳（原子操作）
int try_node_alert_upload(uint32_t id, int interval_sec);

#endif