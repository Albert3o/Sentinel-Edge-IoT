#include "election_manager.h"

ElectionManager::ElectionManager() : 
    _myNodeId(0), _currentRole(ROLE_FOLLOWER),
    _maxFollowerScore(0), _masterScore(0), _masterNodeId(0),
    _lastMasterSeenTime(0), _lastPeerSeenTime(0) {}

void ElectionManager::init(uint32_t myId) {
    _myNodeId = myId;
}

// 计算威胁分：PIR 是门槛（权重2000），LDR 是精度
uint32_t ElectionManager::calculateScore(uint16_t ldr, uint8_t pir) {
    if (pir == 0 && ldr < LDR_THRESHOLD_DARK) return 0; // 漆黑且没动静，分数 0
    return (pir ? 2000 : 0) + ldr;
}

void ElectionManager::updatePeerInfo(uint32_t peerId, uint16_t peerLdr, NodeRole peerRole, uint8_t pir_state) {
    uint32_t peerScore = calculateScore(peerLdr, pir_state);
    _lastPeerSeenTime = millis();

    if (peerRole == ROLE_MASTER) {
        _masterNodeId = peerId;
        _masterScore = peerScore;
        _lastMasterSeenTime = millis();
    } else {
        // 记录普通 Follower 中的最高分
        if (peerScore > _maxFollowerScore) {
            _maxFollowerScore = peerScore;
        }
    }
}

void ElectionManager::update(uint16_t myLdr, bool myMotion) {
    unsigned long now = millis();
    uint32_t myScore = calculateScore(myLdr, myMotion ? 1 : 0);

    // 1. 环境清理：如果太久没听到 Master 说话，Master 记录失效
    bool masterActive = (now - _lastMasterSeenTime < MASTER_TIMEOUT_MS);
    if (!masterActive) {
        _masterScore = 0;
        _masterNodeId = 0;
    }

    // 2. 选举逻辑
    if (_currentRole == ROLE_FOLLOWER) {
        // 晋升 Master 的条件：
        // A. 我得有动静且光照大于设定阈值 (myScore > 2000 + 黑暗阈值)
        // B. 如果当前没 Master，我只要比其他 Follower 强（或相等）就上台
        // C. 如果当前有 Master，我必须比它强出一个“滞后量”，代表小偷更靠近我
        if (myScore > (2000+LDR_THRESHOLD_DARK)) { 
            if (!masterActive) {
                if (myScore >= _maxFollowerScore) setRole(ROLE_MASTER);
            } else {
                if (myScore > _masterScore + ELECTION_HYSTERESIS) setRole(ROLE_MASTER);
            }
        }
    }
    else { // 我已经是 Master
        // 降级为 Follower 的条件：
        // A. 我自己没动静了（小偷走开了）
        if (myScore < (2000+LDR_THRESHOLD_DARK)) {
            setRole(ROLE_FOLLOWER);
        }
        // B. 发现有别的 Master 比我更强（处理多主冲突或正常接力）
        else if (masterActive && _masterNodeId != _myNodeId) {
            if (_masterScore > myScore) setRole(ROLE_FOLLOWER);
        }
    }

    // 断网或者其他结点掉线
    if (now - _lastPeerSeenTime > 2000) {
        _maxFollowerScore = 0;
    }
}

void ElectionManager::setRole(NodeRole newRole) {
    if (_currentRole != newRole) {
        _currentRole = newRole;
        if (_currentRole == ROLE_MASTER) {
            Serial.printf(">>> [ELECTION] Becoming MASTER (Score: %d)\n", _masterScore);
        } else {
            Serial.println(">>> [ELECTION] Stepping down to FOLLOWER");
        }
    }
}