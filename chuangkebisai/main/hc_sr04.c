#include "hc_sr04.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"    // 提供 esp_rom_delay_us
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HC_SR04";

void hc_sr04_init(void) {
    // 配置 TRIG 为输出
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_conf);
    gpio_set_level(TRIG_PIN, 0);

    // 配置 ECHO 为输入
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // 可选，内部上拉提高稳定性
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_conf);

    ESP_LOGI(TAG, "HC-SR04 initialized: TRIG=%d, ECHO=%d", TRIG_PIN, ECHO_PIN);
}

float hc_sr04_get_distance_cm(void) {
    // 发送 10us 高电平触发信号
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // 等待 ECHO 变为高电平（开始计时）
    uint32_t timeout = 10000;  // 10ms 超时
    while (gpio_get_level(ECHO_PIN) == 0 && timeout--) {
        esp_rom_delay_us(1);
    }
    if (timeout == 0) {
        ESP_LOGW(TAG, "ECHO timeout (no rising edge)");
        return -1.0f;
    }

    int64_t start_time = esp_timer_get_time();

    // 等待 ECHO 变为低电平（结束计时）
    timeout = 10000;
    while (gpio_get_level(ECHO_PIN) == 1 && timeout--) {
        esp_rom_delay_us(1);
    }
    if (timeout == 0) {
        ESP_LOGW(TAG, "ECHO timeout (no falling edge)");
        return -1.0f;
    }

    int64_t end_time = esp_timer_get_time();
    int64_t pulse_duration_us = end_time - start_time;

    // 计算距离（cm）
    float distance_cm = (pulse_duration_us * SOUND_SPEED_CM_US) / 2.0f;

    // 过滤异常值
    if (distance_cm < 2.0f || distance_cm > 400.0f) {
        ESP_LOGW(TAG, "Abnormal distance: %.2f cm", distance_cm);
        return -1.0f;
    }

    return distance_cm;
}