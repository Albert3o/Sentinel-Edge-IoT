#ifndef PROTOCOL_H
#define PROTOCOL_H
#define FIRST_VERSION 0x01
#include <stdint.h>

// 强制按 1 字节对齐，防止不同编译器补齐导致数据错乱
#pragma pack(push, 1) // pragma: Pragmatic Information

typedef enum {
    PKT_HEARTBEAT = 0,
    PKT_ALERT = 1,
    PKT_ACK = 2
} PacketType;

typedef enum {
    ROLE_FOLLOWER = 0,
    ROLE_MASTER = 1
} NodeRole;

typedef struct {
    uint8_t  version;      // 协议版本 0x01
    uint8_t  pkt_type;     // PacketType
    uint8_t  node_role;    // NodeRole
    uint32_t node_id;      // 节点唯一ID
    uint32_t uptime_ms;    // 节点运行时间
    
    // 数据载荷
    uint16_t ldr_value;    // LDR 采样值
    uint8_t  pir_state;    // 0:无移动, 1:检测到移动
    uint8_t  severity;     // 0:Normal, 1:Warning, 2:Critical
} IoTProtocolPacket;

#pragma pack(pop) // // 恢复到最近一次push的状态，即1字节对齐

#endif