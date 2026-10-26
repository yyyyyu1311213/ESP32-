#ifndef AUDIO_H
#define AUDIO_H

#include "driver/gpio.h"
#include "driver/i2s.h"

// 麦克风引脚 (INMP441)
#define MIC_DIN_PIN   GPIO_NUM_36
#define MIC_BCK_PIN   GPIO_NUM_37
#define MIC_WS_PIN    GPIO_NUM_35

// MAX98357 引脚
#define AMP_LRC_PIN   GPIO_NUM_40
#define AMP_BCK_PIN   GPIO_NUM_41
#define AMP_DIN_PIN   GPIO_NUM_42

#define SAMPLE_RATE   16000
#define BUFFER_LEN    256

// 初始化音频（麦克风和功放）
void audio_init(void);

// 检测麦克风是否有声音（返回 true 表示音量超过阈值）
bool is_sound_detected(int threshold);

// 播放一个指定频率和时长的提示音（用于回应）
void audio_play_tone(int freq_hz, int duration_ms);

#endif