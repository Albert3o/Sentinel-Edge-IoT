#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "protocol.h"

// 节点状态结构体
typedef struct Node {
    uint32_t node_id;
    uint16_t last_ldr;
    uint8_t  last_pir;
    NodeRole role;
    time_t   last_seen;     // 记录最后一次收到心跳的系统时间
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

#endif