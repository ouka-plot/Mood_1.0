/**
 ****************************************************************************************************
 * @file        audio_denoise.h
 * @brief       频域谱减法降噪模块
 * @details     算法: FFT → 估计噪声谱 → 计算增益 → 应用增益 → IFFT
 *              复用 CMSIS-DSP arm_rfft_fast_f32, 在 TinyML 推理流水线中使用
 ****************************************************************************************************
 */

#ifndef __AUDIO_DENOISE_H
#define __AUDIO_DENOISE_H

#include "arm_math.h"

/* ======================== 配置参数 ======================== */
#define DENOISE_FFT_LEN         512     /* 与MFCC保持一致 */
#define DENOISE_SAMPLE_RATE     16000

/* 谱减法参数 */
#define DENOISE_ALPHA           2.0f    /* 过减因子 (越大降噪越强, 但可能失真) */
#define DENOISE_BETA            0.02f   /* 谱下限 (防止"音乐噪声"伪影) */
#define DENOISE_NOISE_FRAMES    5       /* 开头多少帧用于估计噪声谱 */

/* ======================== 函数接口 ======================== */

/**
 * @brief  初始化降噪模块
 * @note   从CCM heap分配FFT缓冲和噪声谱估计数组
 * @retval 0=成功, -1=内存分配失败
 */
int audio_denoise_init(void);

/**
 * @brief  对一帧音频进行谱减法降噪 (原地处理)
 * @param  frame : 输入/输出 单声道PCM [DENOISE_FFT_LEN] (int16_t)
 * @note   前 DENOISE_NOISE_FRAMES 帧自动用于噪声估计, 不做降噪
 */
void audio_denoise_process(int16_t *frame);

/**
 * @brief  重置噪声估计 (换曲或环境变化时调用)
 */
void audio_denoise_reset(void);

/**
 * @brief  获取模块初始化状态
 * @retval 1=已初始化, 0=未初始化
 */
uint8_t audio_denoise_is_ready(void);

#endif /* __AUDIO_DENOISE_H */
