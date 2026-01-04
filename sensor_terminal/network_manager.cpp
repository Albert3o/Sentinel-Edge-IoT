#include "network_manager.h"

MyNetworkManager::MyNetworkManager() : _nodeId(0), _hasFoundGateway(false) {}

/*
连接wifi
分配唯一结点id
初始化广播地址
*/
void MyNetworkManager::init(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    /*
    假设设备的MAC地址（一共为48位）为：AC:67:B2:E5:12:34，前24位为厂商的代码AC:67:B2，后24位为设备序列号E5:12:34。
    这里我们采用整体的后32位，也就是后8位厂商代码+设备序列号来当结点的唯一标识
    在 ESP32 中，ESP.getEfuseMac() 的实现方式是将 MAC 的第一个字节放在 uint64_t 的最低 8 位（大端字节序）
    用uint64_t来接收会变成：mac = 0x0000_3412_E5B2_67AC
    因此我们要右移16位得到：0x0000_0000_3412_E5B2；然后取uint32_t，就会得到0x3412_E5B2（截断原理：保留低位，丢弃高位）
    */
    uint64_t mac = ESP.getEfuseMac();
    _nodeId = (uint32_t)(mac >> 16);

    _udp.begin(UDP_PORT_BROADCAST);
    _broadcastAddr = IPAddress(255, 255, 255, 255);
}


bool MyNetworkManager::receivePacket(IoTProtocolPacket* pkt) {
    // 1. 检查是否有数据包到达，并获取大小
    int packetSize = _udp.parsePacket();
    
    if (packetSize > 0) {
        // 2. 安全检查：确保收到的包大小符合我们的协议结构体
        if (packetSize == sizeof(IoTProtocolPacket)) {
            // 3. 将 UDP 缓冲区的数据直接读取到指针指向的内存中
            _udp.read((uint8_t*)pkt, sizeof(IoTProtocolPacket));
            
            // 4. 版本校验，确保协议匹配
            if (pkt->version == FIRST_VERSION) {
                return true; // 成功获取有效数据
            }
        } else {
            // 如果包大小不对，清空缓冲区，防止阻塞
            _udp.clear();
        }
    }
    return false; // 没有收到包或包无效
}


void MyNetworkManager::setGatewayIP(IPAddress ip){
    _hasFoundGateway = true;
    _gatewayAddr = ip;
}


// 获取发送者的 IP（用于网关发现）
IPAddress MyNetworkManager::getGatewayIP() {
    return _udp.remoteIP();
}


void MyNetworkManager::broadcastHeartbeat(NodeRole role, uint16_t ldr, uint8_t pir) {
    IoTProtocolPacket pkt;
    pkt.version = FIRST_VERSION;
    pkt.pkt_type = PKT_HEARTBEAT;
    pkt.node_role = role;
    pkt.node_id = _nodeId;
    pkt.uptime_ms = millis();
    pkt.ldr_value = ldr;
    pkt.pir_state = pir;
    pkt.severity = 0; // 心跳默认为 Normal
    // 发送udp数据报
    _udp.beginPacket(_broadcastAddr, UDP_PORT_BROADCAST);
    _udp.write((uint8_t*)&pkt, sizeof(pkt));
    _udp.endPacket();
}

void MyNetworkManager::sendAlertToGateway(uint16_t ldr, uint8_t pir, uint8_t severity) {
    // 此时先向广播地址发送 Alert，直到后续实现“网关发现”逻辑后改为 _gatewayAddr
    IoTProtocolPacket pkt;
    pkt.version = FIRST_VERSION;
    pkt.pkt_type = PKT_ALERT;
    pkt.node_role = ROLE_MASTER;
    pkt.node_id = _nodeId;
    pkt.uptime_ms = millis();
    pkt.ldr_value = ldr;
    pkt.pir_state = pir;
    pkt.severity = severity;
    // 检测是否有rpi地址了
    IPAddress targetIP;
    if (_hasFoundGateway) {
        targetIP = _gatewayAddr; // 已经收到过 RPi 的回复，现在改用私聊（单播）
    } else {
        targetIP = _broadcastAddr; // 还没收到过回复，继续大声广播
    }
    _udp.beginPacket(targetIP, UDP_PORT_GATEWAY);
    _udp.write((uint8_t*)&pkt, sizeof(pkt));
    _udp.endPacket();
}