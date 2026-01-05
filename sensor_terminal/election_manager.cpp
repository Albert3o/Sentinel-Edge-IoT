#include "election_manager.h"

ElectionManager::ElectionManager() : 
    _myNodeId(0), 
    _currentRole(ROLE_FOLLOWER),
    _maxPeerLdr(0),
    _lastPeerSeenTime(0),
    _lastSwitchMillis(0) {}

void ElectionManager::init(uint32_t myId) {
    _myNodeId = myId;
}


void ElectionManager::updatePeerInfo(uint32_t peerId, uint16_t peerLdr, NodeRole peerRole, uint8_t pir_state) {
    // 记录检测到运动的时间，用于判断网络是否断开
    _lastPeerSeenTime = millis(); 

    if (pir_state) {
        // 只有别人在动，这个数据才有竞争意义
        if (peerLdr > _maxPeerLdr) {
            _maxPeerLdr = peerLdr;
        }
        // 记录最后一次看到“有效竞争者”的时间
        _lastMovingPeerSeenTime = millis(); 
    }
}


void ElectionManager::update(uint16_t myLdr, bool myMotion) {
    unsigned long now = millis();

    // 1. 检查冷却时间
    if (now - _lastSwitchMillis < COOLDOWN_MS) {
        return; 
    }

    // 2. 如果全网太久没声音了（比如你是第一个上线的），重置全网最强值
    if (now - _lastPeerSeenTime > 5000) {
        _maxPeerLdr = 0;
        _lastPeerSeenTime = millis();
    }

    // 上次检测到motion+最大LDR的ESP如果5秒没动静了，就重置_maxPeerLdr
    if (now - _lastMovingPeerSeenTime > 5000) {
        _maxPeerLdr = 0;
    }

    // 3. 选举逻辑核心
    if (_currentRole == ROLE_FOLLOWER) {
        // 晋升条件：有移动事件 && 我的亮度比其他检测到移动的esp更大 && 我的亮度大于设定阈值
        if (myMotion && (myLdr > _maxPeerLdr + ELECTION_HYSTERESIS) && (myLdr > LDR_THRESHOLD_DARK)) {
            setRole(ROLE_MASTER);
        }
    } else {
        // 降级条件：没移动了，或者有人的亮度超过我了
        if (!myMotion || (myLdr < _maxPeerLdr)) {
            setRole(ROLE_FOLLOWER);
        }
    }
}

void ElectionManager::setRole(NodeRole newRole) {
    if (_currentRole != newRole) {
        _currentRole = newRole;
        _lastSwitchMillis = millis();
        
        if (_currentRole == ROLE_MASTER) {
            Serial.println(">>> [ROLE] I am now the MASTER!");
            // Master 模式：LED 快速闪烁（在主循环处理）或保持亮起
        } else {
            Serial.println(">>> [ROLE] Stepping down to FOLLOWER.");
        }
    }
}