/**
 ****************************************************************************************************
 * @file        recorder.h
 * @brief       WAV录音模块 - 通过I2S RX采集音频并保存为WAV文件到SD卡
 ****************************************************************************************************
 */

#ifndef __RECORDER_H
#define __RECORDER_H

#include "sys.h"

/* 录音配置 */
#define RECORDER_SAMPLE_RATE        16000       /* 采样率 16kHz */
#define RECORDER_CHANNELS           2           /* 立体声 (I2S固定双声道) */
#define RECORDER_BITS               16          /* 16位采样 */
#define RECORDER_BUF_SIZE           512         /* 单个DMA缓冲区大小(采样点数/通道) */
#define RECORDER_SAVE_DIR           "0:/RECORD" /* 保存目录 */
#define RECORDER_MAX_DURATION       (60*30)     /* 最大录音时长(秒) 30分钟 */

/* 录音状态 */
typedef enum {
    RECORDER_STATE_IDLE = 0,        /* 空闲 (录音界面但未开始) */
    RECORDER_STATE_RECORDING,       /* 正在录音 */
    RECORDER_STATE_PAUSED,          /* 录音暂停 */
    RECORDER_STATE_SAVING,          /* 正在保存WAV头 */
    RECORDER_STATE_DONE             /* 录音完成 */
} RecorderState_t;

/* 函数声明 */
void recorder_init(void);                /* 初始化录音模块(创建目录等) */
uint8_t recorder_start(void);            /* 开始录音, 返回0成功 */
void recorder_pause(void);               /* 暂停录音 */
void recorder_resume(void);              /* 恢复录音 */
uint8_t recorder_stop(void);             /* 停止录音并保存文件, 返回0成功 */
void recorder_abort(void);               /* 放弃录音(不保存) */

RecorderState_t recorder_get_state(void);     /* 获取录音状态 */
uint32_t recorder_get_duration(void);         /* 获取当前录音时长(秒) */
uint32_t recorder_get_data_size(void);        /* 获取已录制数据大小(字节) */

/* 由DMA回调调用(ISR安全) */
void recorder_dma_callback(void);

/* FreeRTOS录音处理(在音频任务中调用) */
void recorder_process(void);

#endif /* __RECORDER_H */
