#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "driver/gpio.h"

// 引脚定义（使用 GPIO10，您可按需修改）
#define LED_GPIO    GPIO_NUM_10

// 初始化 LED
void led_init(void);

// 设置 LED 状态：true 点亮，false 熄灭
void led_set(bool on);

// 闪烁一次：亮 on_ms 毫秒，灭 off_ms 毫秒
void led_blink(int on_ms, int off_ms);

#endif
