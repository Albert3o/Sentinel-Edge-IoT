#include "IPAddress.h"
#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocol.h"
#include "config.h"

// 对应需求中的端口号
#define UDP_PORT_BROADCAST 8888
#define UDP_PORT_GATEWAY   8888

class MyNetworkManager {
public:
    MyNetworkManager();
    void init(const char* ssid, const char* password);
    
    // 发送心跳广播
    void broadcastHeartbeat(NodeRole role, uint16_t ldr, uint8_t pir);
    
    // 向网关单播异常 (仅 Master 调用)
    void sendAlertToGateway(uint16_t ldr, uint8_t pir);

    // 读取其他节点udp广播消息
    bool receivePacket(IoTProtocolPacket* pkt);

    // 当收到rpi广播时获取rpi地址
    IPAddress getGatewayIP();

    // 保存rpi地址
    void setGatewayIP(IPAddress ip);

    uint32_t getNodeId() { return _nodeId; }

private:
    WiFiUDP _udp; // 用来执行 beginPacket()、write() 和 endPacket() 等操作的工具
    uint32_t _nodeId; // 结点唯一标识
    bool _hasFoundGateway; // 是否有rpi地址了
    // IPAddress是专门用来存储和处理IPv4地址的辅助类，内部是一个占用4字节的数组 eg：192，168，1，100
    // 重载了运算符，允许直接用IPAddress(192, 168, 1, 255)
    IPAddress _broadcastAddr;
    IPAddress _gatewayAddr; // 记录收到的网关地址
};

#endif