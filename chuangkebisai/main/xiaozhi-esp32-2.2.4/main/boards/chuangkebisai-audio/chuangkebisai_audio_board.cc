#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "application.h"

extern "C" {
#include "lcd1602.h"
#include "hc_sr04.h"
#include "buzzer.h"
#include "led.h"
}

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <cstdio>
#include <cstring>

#define TAG "ChuangkeBisaiAudio"

class ChuangkeBisaiAudioBoard : public WifiBoard {
private:
    bool lcd_ready_ = false;

    static void MonitorTaskEntry(void* arg) {
        static_cast<ChuangkeBisaiAudioBoard*>(arg)->MonitorTask();
    }

    void UpdateTimerDisplay(float distance, bool present, uint32_t elapsed_seconds) {
        if (!lcd_ready_) {
            return;
        }

        char line1[17];
        char line2[17];
        memset(line1, ' ', sizeof(line1) - 1);
        memset(line2, ' ', sizeof(line2) - 1);
        line1[16] = '\0';
        line2[16] = '\0';

        const uint32_t hours = elapsed_seconds / 3600;
        const uint32_t minutes = (elapsed_seconds / 60) % 60;
        const uint32_t seconds = elapsed_seconds % 60;
        snprintf(line1, sizeof(line1), "Time %02u:%02u:%02u",
                 (unsigned)(hours % 100), (unsigned)minutes, (unsigned)seconds);
        if (present && distance > 0.0f) {
            snprintf(line2, sizeof(line2), "Dist:%4.1fcm", distance);
        } else {
        snprintf(line2, sizeof(line2), "Waiting...");
        }

        for (size_t i = 0; i < 16; ++i) {
            if (line1[i] == '\0') {
                line1[i] = ' ';
            }
            if (line2[i] == '\0') {
                line2[i] = ' ';
            }
        }
        line1[16] = '\0';
        line2[16] = '\0';

        lcd1602_set_cursor(0, 0);
        lcd1602_write_string(line1);
        lcd1602_set_cursor(0, 1);
        lcd1602_write_string(line2);
    }

    void MonitorTask() {
        // Let XiaoZhi finish audio and network initialization before starting
        // the sensor and LCD workload.
        vTaskDelay(pdMS_TO_TICKS(3000));

        bool present = false;
        bool greeting_sent = false;
        bool alarm_sent = false;
        int missing_samples = 0;
        constexpr int kMissedSamplesBeforeReset = 6;
        int alarm_seconds = 0;
        uint32_t elapsed_seconds = 0;

        if (lcd_ready_) {
            lcd1602_clear();
        }
        UpdateTimerDisplay(-1.0f, false, 0);

        while (true) {
            const float distance = hc_sr04_get_distance_cm();
            const bool close = distance > 0.0f && distance < 10.0f;

            if (close) {
                missing_samples = 0;
                if (!present) {
                    present = true;
                    greeting_sent = false;
                    alarm_sent = false;
                    elapsed_seconds = 0;
                    alarm_seconds = 0;
                    ESP_LOGI(TAG, "Person detected at %.1f cm, timer started", distance);
                }
            } else if (present) {
                // HC-SR04 can miss an echo when the person moves or turns.
                // Require several consecutive misses before resetting the timer.
                if (++missing_samples >= kMissedSamplesBeforeReset) {
                    present = false;
                    greeting_sent = false;
                    alarm_sent = false;
                    elapsed_seconds = 0;
                    alarm_seconds = 0;
                    led_set(false);
                    buzzer_off();
                    ESP_LOGI(TAG, "Person left, timer reset");
                }
            }

            if (present) {
                ++elapsed_seconds;

                if (!greeting_sent) {
                    auto& app = Application::GetInstance();
                    if (app.GetDeviceState() == kDeviceStateIdle) {
                        // This follows XiaoZhi's normal wake-word path, so the
                        // server sends the greeting and ordinary conversation
                        // remains unchanged.
                        app.WakeWordInvoke("\xE4\xBD\xA0\xE5\xA5\xBD\xE5\xB0\x8F\xE6\x99\xBA");
                        greeting_sent = true;
                        ESP_LOGI(TAG, "Triggered XiaoZhi greeting");
                    }
                }

                if (elapsed_seconds >= 60 && !alarm_sent) {
                    alarm_sent = true;
                    alarm_seconds = 3;
                    ESP_LOGI(TAG, "Sedentary alarm triggered");
                }
            }

            if (alarm_seconds > 0) {
                led_set(true);
                buzzer_on();
                --alarm_seconds;
            } else {
                led_set(false);
                buzzer_off();
            }

            UpdateTimerDisplay(distance, present, elapsed_seconds);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

public:
    ChuangkeBisaiAudioBoard() {
        ESP_LOGI(TAG, "Board: INMP441 + MAX98357 + sedentary timer");

        lcd_ready_ = lcd1602_init() == ESP_OK;
        led_init();
        buzzer_init();
        hc_sr04_init();

        if (xTaskCreatePinnedToCore(MonitorTaskEntry, "sedentary_timer", 4096, this, 1, nullptr, 1) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create sedentary timer task");
        }
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return Board::GetDisplay();
    }

    virtual Led* GetLed() override {
        return Board::GetLed();
    }
};

DECLARE_BOARD(ChuangkeBisaiAudioBoard);
