#include "dht11.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_err_t dht11_read_raw_data(uint8_t buf[5])
{
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_DATA_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(18));
    gpio_set_level(DHT11_DATA_PIN, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_INPUT);

    int timeout = 0;
    while(gpio_get_level(DHT11_DATA_PIN) == 0) { timeout++; if(timeout > 1000) return ESP_ERR_TIMEOUT; }
    timeout = 0;
    while(gpio_get_level(DHT11_DATA_PIN) == 1) { timeout++; if(timeout > 1000) return ESP_ERR_TIMEOUT; }

    for(int i = 0; i < 5; i++)
    {
        buf[i] = 0;
        for(int j = 0; j < 8; j++)
        {
            timeout = 0;
            while(gpio_get_level(DHT11_DATA_PIN) == 0) { timeout++; if(timeout > 1000) return ESP_ERR_TIMEOUT; }
            esp_rom_delay_us(30);
            if(gpio_get_level(DHT11_DATA_PIN) == 1)
                buf[i] |= (1 << (7 - j));
            timeout = 0;
            while(gpio_get_level(DHT11_DATA_PIN) == 1) { timeout++; if(timeout > 1000) return ESP_ERR_TIMEOUT; }
        }
    }
    if((buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
        return ESP_ERR_INVALID_CRC;
    return ESP_OK;
}

void dht11_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << DHT11_DATA_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);
}

float dht11_read_temperature(void)
{
    uint8_t buf[5];
    if(dht11_read_raw_data(buf) != ESP_OK)
        return -99.0f;
    return (float)buf[2];
}

float dht11_read_humidity(void)
{
    uint8_t buf[5];
    if(dht11_read_raw_data(buf) != ESP_OK)
        return -99.0f;
    return (float)buf[0];
}