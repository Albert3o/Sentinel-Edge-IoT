#include <cstdio>
#include "sensor_manager.h"

SensorManager::SensorManager() : _consecutive_pir_high(0) {
    _currentData = {0, false, false};
}

void SensorManager::init() {
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, LOW);
    analogReadResolution(12); // ESP32 ADC 12位分辨率 (0-4095)
}

void SensorManager::update() {
    // 1. 读取 LDR 模拟值
    _currentData.ldr_value = analogRead(PIN_LDR);

    // 2. 读取 PIR 并进行软件去抖 (PRD V2.1 需求)
    bool raw_pir = digitalRead(PIN_PIR);
    if (raw_pir) {
        _consecutive_pir_high++;
    } else {
        _consecutive_pir_high = 0;
    }

    // 记录旧状态用于对比
    bool old_motion = _currentData.is_motion_detected;
    // 异常判断
    if (_consecutive_pir_high >= PIR_DEBOUNCE_COUNT) {
        _currentData.is_motion_detected = true;
    } else {
        _currentData.is_motion_detected = false;
    }

    // 在状态改变或间隔一段时间时打印
    if (old_motion != _currentData.is_motion_detected) {
        Serial.printf("[Sensor] Motion: %s | LDR: %u\n", 
                      _currentData.is_motion_detected ? "True" : "False", 
                      _currentData.ldr_value);
    }

    // 判定异常逻辑：有移动 且 亮度超过设定的“黑暗环境”上限（即有人在黑暗中开了灯）
    // 注意：这里用 > 还是 < 取决于你 LDR 的电路接法，通常是光越亮数值越大
    if (_currentData.is_motion_detected && _currentData.ldr_value > LDR_THRESHOLD_DARK) {
        _currentData.is_anomalous = true;
    } else {
        _currentData.is_anomalous = false;
    }
}



SensorData SensorManager::getData() {
    return _currentData;
}