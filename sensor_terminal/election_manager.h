#ifndef ELECTION_MANAGER_H
#define ELECTION_MANAGER_H

#include <Arduino.h>
#include "protocol.h"
#include "config.h"

// 选举相关的宏定义
#define ELECTION_HYSTERESIS 50   // 阈值
#define COOLDOWN_MS         5000 // 5秒冷却

class ElectionManager {
public:
    ElectionManager();
    void init(uint32_t myId);
    
    // 核心：处理从网络收到的其他节点的数据
    void updatePeerInfo(uint32_t peerId, uint16_t peerLdr, NodeRole peerRole, uint8_t pir_state);
    
    // 每轮循环根据自身数据更新角色
    void update(uint16_t myLdr, bool myMotion);

    NodeRole getRole() { return _currentRole; }

private:
    uint32_t _myNodeId;
    NodeRole _currentRole;
    
    uint16_t _maxPeerLdr;      // 当前局域网中别人最强的 LDR
    uint32_t _lastPeerSeenTime; // 上次听到别人说话的时间
    uint32_t _lastMovingPeerSeenTime; // 上次PIR=1的esp说话的时间
    unsigned long _lastSwitchMillis; // 上次身份切换的时间点

    void setRole(NodeRole newRole);
};

#endif