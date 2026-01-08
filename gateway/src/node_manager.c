#include <stdio.h>
#include <stdlib.h>
#include "node_manager.h"
#include "utils.h"

static Node *node_list_head = NULL;
static pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_node_manager() {
    pthread_mutex_init(&table_mutex, NULL);
}


/*
当 gateway_core 的 epoll 收到一个 UDP 包时：
查表：遍历链表，看 node_id 是否已存在。
更新：如果存在，就把最新的 LDR、PIR 数值填进去，并刷新 last_seen 为当前系统时间。
新增：如果不存在，说明是个新节点，立刻 malloc 内存创建一个新节点插进表头。
*/
void update_node(uint32_t id, uint16_t ldr, uint8_t pir, NodeRole role) {
    pthread_mutex_lock(&table_mutex);

    Node *curr = node_list_head;
    while (curr) {
        if (curr->node_id == id) {
            // 找到已知节点，更新数据
            curr->last_ldr = ldr;
            curr->last_pir = pir;
            curr->role = role;
            curr->last_seen = time(NULL); // 表示刚刚还在
            pthread_mutex_unlock(&table_mutex);
            return;
        }
        curr = curr->next;
    }

    // 未找到节点，新建并插入链表头部（头插法）
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->node_id = id;
    new_node->last_ldr = ldr;
    new_node->last_pir = pir;
    new_node->role = role;
    new_node->last_alert_time = 0; // 表示还没出现过
    new_node->last_seen = time(NULL);
    new_node->next = node_list_head;
    node_list_head = new_node;
    log_message(LOG_INFO, "[NodeMgr] New node detected: 0x%X", id);
    pthread_mutex_unlock(&table_mutex);
}

/*
遍历结点链表，检查是否有超时结点。若有，就从结点链表删除
*/
void check_node_timeouts(int timeout_sec) {
    pthread_mutex_lock(&table_mutex);
    
    time_t now = time(NULL);
    Node **curr = &node_list_head;

    while (*curr) {
        Node *entry = *curr;
        if (now - entry->last_seen > timeout_sec) {
            // 超时，从链表中移除
            log_message(LOG_INFO, "[NodeMgr] Node 0x%X timed out, removing...", entry->node_id);
            *curr = entry->next;
            free(entry);
        } else {
            curr = &entry->next;
        }
    }
  
    pthread_mutex_unlock(&table_mutex);
}


void dump_node_table() {
    pthread_mutex_lock(&table_mutex);
    log_message(LOG_INFO,  "\n--- [NodeMgr] Active Nodes ---");
    Node *curr = node_list_head;
    while (curr) {
        log_message(LOG_INFO,  
               "[NodeMgr] ID: 0x%X | Role: %s | LDR: %d | PIR: %d | Score: %d\n", 
               curr->node_id, 
               (curr->role == ROLE_MASTER ? "MASTER" : "FOLLOWER"),
               curr->last_ldr, 
               curr->last_pir,
               curr->last_ldr+2000*curr->last_pir);
        
        curr = curr->next;
    }
    log_message(LOG_INFO,  "--------------------");
    pthread_mutex_unlock(&table_mutex);
}


// 检查并更新上传时间戳（原子操作）
// 返回 1 表示允许上传，返回 0 表示被限流
int try_node_alert_upload(uint32_t id, int interval_sec) {
    pthread_mutex_lock(&table_mutex); 
    Node *curr = node_list_head;
    time_t now = time(NULL);

    while (curr) {
        if (curr->node_id == id) {
            if (now - curr->last_alert_time >= interval_sec) {
                curr->last_alert_time = now; // 在锁内直接更新
                pthread_mutex_unlock(&table_mutex);
                return 1; 
            }
            pthread_mutex_unlock(&table_mutex);
            return 0; // 被限流
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&table_mutex);
    return 0; // 未找到节点
}