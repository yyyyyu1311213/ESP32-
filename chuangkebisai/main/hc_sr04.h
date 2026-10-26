#ifndef HC_SR04_H
#define HC_SR04_H

#include "driver/gpio.h"

// 根据您最新的接线修改
#define TRIG_PIN   GPIO_NUM_2    // TRIG 接 GPIO2
#define ECHO_PIN   GPIO_NUM_3    // ECHO 接 GPIO3

// 声速（cm/us）
#define SOUND_SPEED_CM_US  0.0343f

void hc_sr04_init(void);
float hc_sr04_get_distance_cm(void);

#endif