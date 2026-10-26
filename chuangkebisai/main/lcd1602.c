#include "lcd1602.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "LCD1602";
static uint8_t lcd_backlight = 1;
static i2c_master_bus_handle_t lcd_i2c_bus;
static i2c_master_dev_handle_t lcd_i2c_device;

// 检查I2C设备是否存在
static bool lcd_check_device(void) {
    esp_err_t ret = i2c_master_probe(lcd_i2c_bus, LCD1602_ADDR, 100);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LCD device found at 0x%02X", LCD1602_ADDR);
        return true;
    } else {
        ESP_LOGE(TAG, "LCD device NOT found at 0x%02X", LCD1602_ADDR);
        return false;
    }
}

static esp_err_t lcd_i2c_write(const uint8_t *data, size_t length) {
    esp_err_t ret = ESP_FAIL;
    for (int attempt = 0; attempt < 3; ++attempt) {
        ret = i2c_master_transmit(lcd_i2c_device, data, length, 20);
        if (ret == ESP_OK) {
            return ESP_OK;
        }
        ESP_LOGW(TAG, "I2C LCD write failed (%d/3): %s", attempt + 1, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    // A short power or EMI disturbance can leave the ESP32 I2C FSM in a
    // non-DONE state even after the LCD has released the bus.
    esp_err_t reset_ret = i2c_master_bus_reset(lcd_i2c_bus);
    ESP_LOGW(TAG, "I2C LCD bus reset: %s", esp_err_to_name(reset_ret));
    if (reset_ret == ESP_OK) {
        ret = i2c_master_transmit(lcd_i2c_device, data, length, 20);
    }
    return ret;
}

static esp_err_t lcd_i2c_write_byte(uint8_t data) {
    return lcd_i2c_write(&data, 1);
}

static void lcd_write_nibble(uint8_t nibble, uint8_t mode) {
    const uint8_t backlight = lcd_backlight ? 0x08 : 0x00;
    const uint8_t byte = (nibble & 0xF0) | mode | backlight;
    const uint8_t sequence[] = {byte, (uint8_t)(byte | 0x04), (uint8_t)(byte & ~0x04)};

    // Keep the E pulse in one I2C transaction so no unrelated transfer can
    // split a nibble into an invalid command.
    lcd_i2c_write(sequence, sizeof(sequence));
    esp_rom_delay_us(50);
}

static void lcd_write_4bits(uint8_t data, uint8_t mode) {
    lcd_write_nibble(data & 0xF0, mode);
    lcd_write_nibble((data << 4) & 0xF0, mode);
}

void lcd1602_write_cmd(uint8_t cmd) {
    lcd_write_4bits(cmd, 0x00);
}

void lcd1602_write_data(uint8_t data) {
    lcd_write_4bits(data, 0x01);
}

void lcd1602_write_string(const char *str) {
    while (*str) {
        lcd1602_write_data(*str++);
        esp_rom_delay_us(100);
    }
}

void lcd1602_set_cursor(uint8_t col, uint8_t row) {
    uint8_t addr = (row == 0) ? (0x00 + col) : (0x40 + col);
    lcd1602_write_cmd(0x80 | addr);
}

void lcd1602_clear(void) {
    lcd1602_write_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void lcd1602_set_backlight(uint8_t on) {
    lcd_backlight = on ? 1 : 0;
    lcd_i2c_write_byte(0x00 | (lcd_backlight ? 0x08 : 0x00));
}

esp_err_t lcd1602_init(void) {
    ESP_LOGI(TAG, "LCD1602 Init Start");

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_config, &lcd_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD1602_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ret = i2c_master_bus_add_device(lcd_i2c_bus, &device_config, &lcd_i2c_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD I2C device initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 检查设备（假设I2C驱动已安装）
    if (!lcd_check_device()) {
        return ESP_FAIL;
    }

    lcd_backlight = 1;

    // 复位序列
    for (int i = 0; i < 3; i++) {
        lcd_write_nibble(0x30, 0x00);
        if (i == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
        } else {
            esp_rom_delay_us(150);
        }
    }

    lcd_write_nibble(0x20, 0x00);
    esp_rom_delay_us(150);

    lcd1602_write_cmd(0x28);
    esp_rom_delay_us(150);
    lcd1602_write_cmd(0x08);
    esp_rom_delay_us(150);
    lcd1602_clear();
    lcd1602_write_cmd(0x0C);
    lcd1602_write_cmd(0x06);

    ESP_LOGI(TAG, "LCD1602 Init Complete");
    return ESP_OK;
}
