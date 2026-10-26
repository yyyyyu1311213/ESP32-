#ifndef DHT11_H
#define DHT11_H

#include "driver/gpio.h"
#include "esp_err.h"

// 硬件引脚
#define DHT11_DATA_PIN  GPIO_NUM_4

void dht11_init(void);
// 返回值：读取失败返回-99.0f
float dht11_read_temperature(void);
float dht11_read_humidity(void);

#endif