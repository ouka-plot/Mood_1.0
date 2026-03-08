/*
 * @Author: oukaa 3328236081@qq.com
 * @Date: 2026-03-05 21:03:08
 * @LastEditors: oukaa 3328236081@qq.com
 * @LastEditTime: 2026-03-05 21:20:25
 * @FilePath: \Mood_1.0\User\APP\audio_monitor.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 ****************************************************************************************************
 * @file        audio_monitor.h
 * @brief       音频监听模块 - 用于检测高频声音（如婴儿哭声）
 ****************************************************************************************************
 */

#ifndef __AUDIO_MONITOR_H
#define __AUDIO_MONITOR_H

#include "sys.h"

/* 音频监听配置 */
#define AUDIO_MONITOR_SAMPLE_RATE       16000       /* 采样率 16kHz (足够检测高频声音) */
#define AUDIO_MONITOR_BUF_SIZE          512         /* 单个缓冲区大小 (采样点数) */
#define AUDIO_MONITOR_CHANNELS          2           /* 立体声 */

/* 检测阈值配置 */
#define AUDIO_HIGH_FREQ_THRESHOLD       3000        /* 高频阈值 (Hz) - 婴儿哭声通常在300-600Hz基频，但谐波可达2-4kHz */
#define AUDIO_ENERGY_THRESHOLD          50000       /* 能量阈值 - 用于判断是否有显著声音 */
#define AUDIO_ZCR_THRESHOLD             100         /* 过零率阈值 - 高频信号过零率高 */
#define AUDIO_DETECTION_COUNT           3           /* 连续检测次数 - 避免误报 */

/* 监听模式 */
typedef enum {
    AUDIO_MONITOR_MODE_OFF = 0,         /* 关闭监听 */
    AUDIO_MONITOR_MODE_BACKGROUND,      /* 后台监听(与播放同时进行) */
    AUDIO_MONITOR_MODE_ONLY             /* 仅监听(不播放音乐) */
} AudioMonitorMode_t;

/* 检测结果回调函数类型 */
typedef void (*AudioDetectCallback_t)(uint8_t detected, uint32_t energy, uint16_t zcr);

/* 函数声明 */
void audio_monitor_init(void);                                    /* 初始化音频监听 */
void audio_monitor_start(AudioMonitorMode_t mode);                /* 开始监听 */
void audio_monitor_stop(void);                                    /* 停止监听 */
void audio_monitor_set_callback(AudioDetectCallback_t callback);  /* 设置检测回调 */
void audio_monitor_set_threshold(uint32_t energy_th, uint16_t zcr_th);  /* 设置检测阈值 */

/* 获取状态 */
AudioMonitorMode_t audio_monitor_get_mode(void);                  /* 获取当前模式 */
uint8_t audio_monitor_is_detected(void);                          /* 是否检测到高频声音 */
uint32_t audio_monitor_get_energy(void);                          /* 获取当前能量值 */
uint16_t audio_monitor_get_zcr(void);                             /* 获取当前过零率 */

/* FreeRTOS任务函数 */
void vAudioMonitor_Task(void *pvParameters);                      /* 音频监听任务 */

#endif /* __AUDIO_MONITOR_H */
