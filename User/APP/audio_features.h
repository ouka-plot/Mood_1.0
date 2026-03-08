/**
 ****************************************************************************************************
 * @file        audio_features.h
 * @brief       MFCC 特征提取模块 - 基于 CMSIS-DSP arm_mfcc_f32
 * @details     从16kHz单声道音频帧中提取MFCC特征，供TinyML推理使用
 ****************************************************************************************************
 */

#ifndef __AUDIO_FEATURES_H
#define __AUDIO_FEATURES_H

#include "arm_math.h"

/* ======================== 配置参数 ======================== */
#define MFCC_FFT_LEN            512     /* FFT点数 (512样本 = 32ms @ 16kHz) */
#define MFCC_NUM_MEL_FILTERS    20      /* Mel滤波器个数 */
#define MFCC_NUM_COEFFS         13      /* 输出MFCC系数个数 */
#define MFCC_SAMPLE_RATE        16000   /* 采样率 Hz */
#define MFCC_FREQ_MIN           80.0f   /* Mel滤波器最低频率 Hz */
#define MFCC_FREQ_MAX           8000.0f /* Mel滤波器最高频率 Hz (Nyquist) */

/* 1秒特征矩阵参数 */
#define MFCC_FRAMES_PER_SEC     15      /* 每次推理帧数 (8000/512 ≈ 15.6) */
#define MFCC_FEATURE_SIZE       (MFCC_FRAMES_PER_SEC * MFCC_NUM_COEFFS)  /* 403 */

/* ======================== 函数接口 ======================== */

/**
 * @brief  初始化MFCC特征提取模块
 * @note   从CCM heap分配工作缓冲区，计算Mel滤波器系数
 *         必须在FreeRTOS调度器启动后调用(需要pvPortMalloc)
 * @retval 0=成功, -1=内存分配失败
 */
int audio_features_init(void);

/**
 * @brief  对一帧音频计算MFCC特征
 * @param  mono_pcm: 输入单声道PCM数据 [MFCC_FFT_LEN] (int16_t)
 * @param  mfcc_out: 输出MFCC系数 [MFCC_NUM_COEFFS] (float32_t)
 * @note   每次调用处理32ms音频，输出13个MFCC系数
 */
void audio_features_compute_frame(const int16_t *mono_pcm, float32_t *mfcc_out);

/**
 * @brief  获取MFCC模块初始化状态
 * @retval 1=已初始化, 0=未初始化
 */
uint8_t audio_features_is_ready(void);

#endif /* __AUDIO_FEATURES_H */
