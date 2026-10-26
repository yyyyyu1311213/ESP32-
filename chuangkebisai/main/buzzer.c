#include "buzzer.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";

void buzzer_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    buzzer_off();   // 初始关闭
    ESP_LOGI(TAG, "Buzzer initialized on GPIO%d (active HIGH)", BUZZER_GPIO);
}

void buzzer_on(void) {
#if (BUZZER_ACTIVE_LEVEL == 1)
    gpio_set_level(BUZZER_GPIO, 1);   // 高电平响
#else
    gpio_set_level(BUZZER_GPIO, 0);
#endif
}

void buzzer_off(void) {
#if (BUZZER_ACTIVE_LEVEL == 1)
    gpio_set_level(BUZZER_GPIO, 0);   // 低电平关
#else
    gpio_set_level(BUZZER_GPIO, 1);
#endif
}