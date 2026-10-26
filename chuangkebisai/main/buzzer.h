#ifndef BUZZER_H
#define BUZZER_H

#include "driver/gpio.h"

#define BUZZER_GPIO   GPIO_NUM_13     // 正极接 GPIO13，负极接 GND

#define BUZZER_ACTIVE_LEVEL 1         // 高电平有效

void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);

#endif