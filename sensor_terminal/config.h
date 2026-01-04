#ifndef CONFIG_H
#define CONFIG_H

// 硬件引脚分配
#define PIN_LDR          34  // ADC1_CH6
#define PIN_PIR          27  // 数字输入
#define PIN_LED_STATUS    2  // 板载LED

// 采样设置
#define SAMPLING_INTERVAL_MS 500  // 500ms 采样一次

// 选举与判定阈值
#define LDR_THRESHOLD_DARK   1500 // 黑暗阈值（示例值，需根据环境调整）
#define PIR_DEBOUNCE_COUNT   2    // 连续 2 次 HIGH 判定为有效

// 版本类型
#define FIRST_VERSION 0x01
#endif