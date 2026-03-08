/**
 ****************************************************************************************************
 * @file        audio_ringbuf.h
 * @brief       音频环形缓冲区 - 连接DMA采集与TinyML推理
 * @details     1秒环形缓冲(16000样本), 从CCM heap分配, 单生产者单消费者无锁设计
 *              生产者: audio_monitor DMA回调(每次写512样本)
 *              消费者: TinyML推理任务(每次读16000样本快照)
 ****************************************************************************************************
 */

#ifndef __AUDIO_RINGBUF_H
#define __AUDIO_RINGBUF_H

#include "sys.h"

/* ======================== 配置参数 ======================== */
#define RINGBUF_SAMPLE_RATE     16000   /* 采样率 Hz */
#define RINGBUF_DURATION_MS     500     /* 缓冲时长 ms (0.5秒, 节省内存) */
#define RINGBUF_SIZE            (RINGBUF_SAMPLE_RATE * RINGBUF_DURATION_MS / 1000)  /* 8000 样本 = 16KB */

/* ======================== 函数接口 ======================== */

/**
 * @brief  初始化环形缓冲区
 * @note   从CCM heap (pvPortMalloc) 分配 RINGBUF_SIZE * sizeof(int16_t) = 16KB
 *         必须在FreeRTOS调度器启动后调用
 * @retval 0=成功, -1=内存分配失败
 */
int audio_ringbuf_init(void);

/**
 * @brief  向环形缓冲写入单声道采样数据 (由DMA回调/audio_monitor调用)
 * @param  data    : 单声道int16_t数据指针
 * @param  samples : 采样点数 (通常512)
 * @note   可在ISR上下文调用, 内部无阻塞操作
 */
void audio_ringbuf_write(const int16_t *data, uint32_t samples);

/**
 * @brief  读取最近音频快照 (由推理任务调用)
 * @param  out : 输出缓冲区, 大小 >= RINGBUF_SIZE * sizeof(int16_t)
 * @note   将环形缓冲中最近采样按时间顺序拷贝到out
 *         调用者负责分配out缓冲区
 */
void audio_ringbuf_snapshot(int16_t *out);

/**
 * @brief  从快照数据中获取指定帧 (直接拷贝到调用者缓冲)
 * @param  frame_idx   : 帧索引 (0 = 最老帧)
 * @param  frame_size  : 帧大小(样本数)
 * @param  out         : 输出缓冲 [frame_size]
 * @note   内部从ringbuf按偏移拷贝, 处理回绕
 */
void audio_ringbuf_read_frame(uint32_t frame_idx, uint32_t frame_size, int16_t *out);

/**
 * @brief  获取已写入的总采样数
 * @retval 累积写入采样数 (用于判断是否已积累1秒数据)
 */
uint32_t audio_ringbuf_get_total_written(void);

/**
 * @brief  检查是否已积累至少1秒音频数据
 * @retval 1=已满1秒, 0=不足
 */
uint8_t audio_ringbuf_is_ready(void);

/**
 * @brief  复位环形缓冲区 (清零写指针和计数器)
 */
void audio_ringbuf_reset(void);

#endif /* __AUDIO_RINGBUF_H */
