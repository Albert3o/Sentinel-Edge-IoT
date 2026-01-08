#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "sensor_manager.h"
#include "network_manager.h"
#include "election_manager.h"

// wifi
const char* ssid="264 Google";
const char* password="26426492126";

// 任务句柄与锁
TaskHandle_t TaskNetHandle;
TaskHandle_t TaskLogicHandle;
SemaphoreHandle_t xTableMutex; 

// 实例化模块
SensorManager sensorMgr;
MyNetworkManager netMgr;
ElectionManager electMgr;

void setup() {
    Serial.begin(115200);
    // 创建互斥锁
    xTableMutex = xSemaphoreCreateMutex();
    // 初始化硬件与网络
    sensorMgr.init();
    netMgr.init(ssid, password);
    electMgr.init(netMgr.getNodeId());

    // 创建网络接收任务 (固定在 Core 0)
    xTaskCreatePinnedToCore(
        NetworkTask,   // 任务函数
        "TaskNet",         // 任务名
        8192,              // 栈大小
        NULL,              // 参数
        2,                 // 优先级 (较高，防止丢包)
        &TaskNetHandle,    // 句柄
        0                  // 运行核心
    );

    // 创建逻辑处理任务 (固定在 Core 1)
    xTaskCreatePinnedToCore(
        LogicTask,
        "TaskLogic",
        8192,
        NULL,
        1,
        &TaskLogicHandle,
        1
    );
}

void loop() {
    // Arduino 默认的 loop 任务在 Core 1 运行
    // 既然用了 RTOS 任务，这里可以留空或做极低频的监控
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// --- 任务 A：网络接收处理 (Core 0) ---
void NetworkTask(void * pvParameters) {
    IoTProtocolPacket iotPacket;
    while(1) {
        if (netMgr.receivePacket(&iotPacket)) {
            if (xSemaphoreTake(xTableMutex, pdMS_TO_TICKS(10))) {
                
                // 优先处理网关确认包（由于网关 ID 固定，直接根据类型判断）
                if (iotPacket.pkt_type == PKT_ACK) {
                    netMgr.setGatewayIP(netMgr.getGatewayIP()); 
                    // printf("Gateway Found at: %s\n", netMgr.getGatewayIP().toString().c_str());
                } 
                // 其次处理其他节点发来的选举信息包
                else if (iotPacket.node_id != netMgr.getNodeId()) {
                    electMgr.updatePeerInfo(iotPacket.node_id, iotPacket.ldr_value, 
                                          (NodeRole)iotPacket.node_role, iotPacket.pir_state);
                }
                
                xSemaphoreGive(xTableMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}

// --- 任务 B：传感器与选举逻辑 (Core 1) ---
void LogicTask(void * pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount(); // 返回自系统启动以来经过的 Tick 数（FreeRTOS 版本的 millis()）
    const TickType_t xFrequency = pdMS_TO_TICKS(SAMPLING_INTERVAL_MS);

    while(1) {
        // 1. 传感器采样
        sensorMgr.update();
        SensorData sData = sensorMgr.getData();

        // 2. 选举逻辑 (需要锁，因为需要读取 PeerInfo)
        if (xSemaphoreTake(xTableMutex, pdMS_TO_TICKS(50))) {
            electMgr.update(sData.ldr_value, sData.is_motion_detected);
            NodeRole myRole = electMgr.getRole();
            xSemaphoreGive(xTableMutex);

            // 3. 网络发送行为
            netMgr.broadcastHeartbeat(myRole, sData.ldr_value, sData.is_motion_detected);
            if (myRole == ROLE_MASTER && sData.is_anomalous) {
                netMgr.sendAlertToGateway(sData.ldr_value, sData.is_motion_detected);
            }

            // 4. LED 可视化逻辑
            updateLED(myRole, sData.is_anomalous);
        }

        // 精确控制周期
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        
    }
}

void updateLED(NodeRole role, bool isAnomalous) {
    if (role == ROLE_MASTER) {
        digitalWrite(PIN_LED_STATUS, (millis() / 200) % 2); 
    } else {
        digitalWrite(PIN_LED_STATUS, isAnomalous ? HIGH : LOW);
    }
}