#ifndef ELECTION_MANAGER_H
#define ELECTION_MANAGER_H

#include <Arduino.h>
#include "protocol.h"
#include "config.h"

#define ELECTION_HYSTERESIS 30   // 抢占 Master 所需的额外分数（防止抖动）
#define MASTER_TIMEOUT_MS   3000 // 3秒没听到 Master 说话则认为其离线

class ElectionManager {
public:
    ElectionManager();
    void init(uint32_t myId);
    void updatePeerInfo(uint32_t peerId, uint16_t peerLdr, NodeRole peerRole, uint8_t pir_state);
    void update(uint16_t myLdr, bool myMotion);
    NodeRole getRole() { return _currentRole; }

private:
    uint32_t _myNodeId;
    NodeRole _currentRole;
    
    // 竞争状态
    uint32_t _maxFollowerScore;    // 存其他 Follower 的最高分
    uint32_t _masterScore;         // 存当前 Master 的分数
    uint32_t _masterNodeId;        // 当前 Master 的 ID
    uint32_t _lastMasterSeenTime;  // Master 最后出现时间
    uint32_t _lastPeerSeenTime;    // 任意邻居最后出现时间

    uint32_t calculateScore(uint16_t ldr, uint8_t pir);
    void setRole(NodeRole newRole);
};

#endif