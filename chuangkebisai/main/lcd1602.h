#ifndef LCD1602_H
#define LCD1602_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

// I2C引脚配置
#define I2C_MASTER_SCL_IO      GPIO_NUM_9
#define I2C_MASTER_SDA_IO      GPIO_NUM_8
#define I2C_MASTER_NUM         I2C_NUM_1
#define I2C_MASTER_FREQ_HZ     50000        // 降低频率

#define LCD1602_ADDR           0x27         // 或0x3F

esp_err_t lcd1602_init(void);
void lcd1602_write_cmd(uint8_t cmd);
void lcd1602_write_data(uint8_t data);
void lcd1602_write_string(const char *str);
void lcd1602_set_cursor(uint8_t col, uint8_t row);
void lcd1602_clear(void);
void lcd1602_set_backlight(uint8_t on);

#endif
