#include "led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LED";

void led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 初始熄灭
    gpio_set_level(LED_GPIO, 0);
    ESP_LOGI(TAG, "LED initialized on GPIO%d", LED_GPIO);
}

void led_set(bool on) {
    gpio_set_level(LED_GPIO, on ? 1 : 0);
}

void led_blink(int on_ms, int off_ms) {
    led_set(true);
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    led_set(false);
    vTaskDelay(pdMS_TO_TICKS(off_ms));
}