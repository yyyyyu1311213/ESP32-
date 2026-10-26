#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "lcd1602.h"
#include "led.h"
#include "buzzer.h"
#include "hc_sr04.h"
#include "xiaozhuyin"

static const char *TAG = "MAIN";

// 计时变量
static int hours = 0, minutes = 0, seconds = 0;

// 警报标志
static bool alarm_active = false;
static int alarm_counter = 0;

// 超声波测距结果
static float last_distance = 0.0f;
static bool distance_valid = false;

// 构建时间字符串
static void build_time_str(char *buf, int h, int m, int s) {
    buf[0] = ' ';
    buf[1] = ' ';
    buf[2] = '0' + (h / 10);
    buf[3] = '0' + (h % 10);
    buf[4] = ':';
    buf[5] = '0' + (m / 10);
    buf[6] = '0' + (m % 10);
    buf[7] = ':';
    buf[8] = '0' + (s / 10);
    buf[9] = '0' + (s % 10);
    buf[10] = ' ';
    buf[11] = ' ';
    buf[12] = '\0';
}

// 更新LCD显示（第一行显示时间，第二行显示距离）
static void update_lcd(void) {
    char line1[17];
    char line2[17];

    build_time_str(line1, hours, minutes, seconds);

    if (distance_valid) {
        snprintf(line2, sizeof(line2), "Dist: %.1f cm   ", last_distance);
    } else {
        snprintf(line2, sizeof(line2), "Distance: ---   ");
    }

    lcd1602_set_cursor(0, 0);
    lcd1602_write_string(line1);
    lcd1602_set_cursor(0, 1);
    lcd1602_write_string(line2);
}

// 超声波测距任务（每 500ms 测一次）
static void distance_task(void *pvParameters) {
    while (1) {
        float dist = hc_sr04_get_distance_cm();
        if (dist > 0) {
            last_distance = dist;
            distance_valid = true;
        } else {
            distance_valid = false;
        }
        // 测距间隔 500ms，避免频繁触发
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Integrated Timer with Ultrasonic...");

    // 1. 初始化 I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    // 2. 初始化外设
    if (lcd1602_init() != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed!");
        return;
    }
    led_init();
    buzzer_init();
    hc_sr04_init();

    // 3. 创建超声波测距任务
    xTaskCreate(distance_task, "distance_task", 2048, NULL, 5, NULL);

    // 4. 显示初始界面
    lcd1602_clear();
    lcd1602_set_cursor(0, 0);
    lcd1602_write_string("  Timer Start  ");
    lcd1602_set_cursor(0, 1);
    lcd1602_write_string("  00:00:00  ");
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd1602_clear();

    // 5. 主循环（每秒执行一次）
    while (1) {
        // 更新时间
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) hours = 0;
            }

            // 触发警报（整分钟）
            alarm_active = true;
            alarm_counter = 0;
            ESP_LOGI(TAG, "Alarm triggered at %02d:%02d:00", hours, minutes);
        }

        // 处理警报状态
        if (alarm_active) {
            static bool led_state = false;
            led_state = !led_state;
            led_set(led_state);
            buzzer_on();

            alarm_counter++;
            if (alarm_counter >= 3) {
                alarm_active = false;
                led_set(false);
                buzzer_off();
                ESP_LOGI(TAG, "Alarm stopped");
            }
        } else {
            led_set(false);
            buzzer_off();
        }

        // 更新 LCD（时间 + 距离）
        update_lcd();

        // 每分钟整点打印一次时间
        if (seconds == 0) {
            ESP_LOGI(TAG, "Time: %02d:%02d:%02d", hours, minutes, seconds);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}+