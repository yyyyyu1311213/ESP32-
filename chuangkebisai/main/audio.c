#include "audio.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG = "AUDIO";
static i2s_port_t mic_port = I2S_NUM_0;   // 麦克风输入
static i2s_port_t amp_port = I2S_NUM_1;   // 功放输出

void audio_init(void) {
    // ---------- 麦克风（输入） ----------
    i2s_config_t mic_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    i2s_pin_config_t mic_pin = {
        .bck_io_num = MIC_BCK_PIN,
        .ws_io_num = MIC_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_DIN_PIN,
    };

    ESP_ERROR_CHECK(i2s_driver_install(mic_port, &mic_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(mic_port, &mic_pin));
    ESP_ERROR_CHECK(i2s_set_clk(mic_port, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO));

    // ---------- 功放（输出） ----------
    i2s_config_t amp_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    i2s_pin_config_t amp_pin = {
        .bck_io_num = AMP_BCK_PIN,
        .ws_io_num = AMP_LRC_PIN,
        .data_out_num = AMP_DIN_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    ESP_ERROR_CHECK(i2s_driver_install(amp_port, &amp_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(amp_port, &amp_pin));
    ESP_ERROR_CHECK(i2s_set_clk(amp_port, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO));

    ESP_LOGI(TAG, "Audio ready: MIC on I2S%d, AMP on I2S%d", mic_port, amp_port);
}

// 检测声音：读取一个样本，判断绝对值是否大于阈值
bool is_sound_detected(int threshold) {
    int16_t sample = 0;
    size_t bytes_read = 0;
    esp_err_t err = i2s_read(mic_port, &sample, sizeof(int16_t), &bytes_read, pdMS_TO_TICKS(10));
    if (err != ESP_OK || bytes_read == 0) {
        return false;
    }
    if (sample < 0) sample = -sample;
    return sample > threshold;
}

// 播放提示音（生成正弦波）
void audio_play_tone(int freq_hz, int duration_ms) {
    int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
    if (num_samples <= 0) return;

    int16_t *samples = malloc(num_samples * sizeof(int16_t));
    if (samples == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return;
    }

    float step = 2.0f * M_PI * freq_hz / SAMPLE_RATE;
    float phase = 0.0f;
    for (int i = 0; i < num_samples; i++) {
        samples[i] = (int16_t)(16384 * sinf(phase));
        phase += step;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }

    // 分块发送
    int sent = 0;
    while (sent < num_samples) {
        int chunk = BUFFER_LEN;
        if (sent + chunk > num_samples) chunk = num_samples - sent;
        size_t bytes_written = 0;
        esp_err_t err = i2s_write(amp_port, samples + sent, chunk * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error");
            break;
        }
        sent += chunk;
    }

    free(samples);
    // 清空残留数据
    i2s_zero_dma_buffer(amp_port);
}