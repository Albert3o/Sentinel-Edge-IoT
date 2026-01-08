#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "config.h"

// 定义传感器数据结构
typedef struct {
    uint16_t ldr_value;
    bool     is_motion_detected;
    bool     is_anomalous;
} SensorData;

class SensorManager {
public:
    SensorManager();
    void init();
    void update();          // 核心采样逻辑
    SensorData getData();   // 获取最新处理后的数据

private:
    SensorData _currentData;
    int _consecutive_pir_high; // PIR 连续计数器
};

#endif