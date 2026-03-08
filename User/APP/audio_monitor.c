/**
 ****************************************************************************************************
 * @file        audio_monitor.c
 * @brief       音频监听模块实现 - 用于检测高频声音（如婴儿哭声）
 * 
 * @note        检测算法说明：
 *              1. 能量检测：计算音频帧的能量，判断是否有显著声音
 *              2. 过零率(ZCR)：高频信号的过零率较高
 *              3. 连续检测：需要连续多帧检测到才确认，避免误报
 * 
 *              婴儿哭声特征：
 *              - 基频：300-600Hz
 *              - 谐波：可达2-4kHz
 *              - 持续时间：通常1-2秒一个周期
 *              - 能量：中等到较高
 ****************************************************************************************************
 */

#include "./APP/audio_monitor.h"
#include "./APP/audio_ringbuf.h"
#include "./BSP/I2S/i2s.h"
#include "./BSP/ES8388/es8388.h"
#include "./SYSTEM/delay/delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/***************************************************************************************
 *                                    私有变量
 ***************************************************************************************/

/* 双缓冲区 (DMA使用) */
static int16_t s_rx_buf0[AUDIO_MONITOR_BUF_SIZE * AUDIO_MONITOR_CHANNELS];
static int16_t s_rx_buf1[AUDIO_MONITOR_BUF_SIZE * AUDIO_MONITOR_CHANNELS];

/* 处理缓冲区 (单声道提取) */
static int16_t s_process_buf[AUDIO_MONITOR_BUF_SIZE];

/* 状态变量 */
static volatile AudioMonitorMode_t s_monitor_mode = AUDIO_MONITOR_MODE_OFF;
static volatile uint8_t s_buf_ready = 0;            /* 缓冲区就绪标志 */
static volatile uint8_t s_current_buf = 0;          /* 当前DMA使用的缓冲区 */

/* 检测结果 */
static volatile uint8_t s_detected = 0;             /* 检测到高频声音标志 */
static volatile uint32_t s_current_energy = 0;      /* 当前能量值 */
static volatile uint16_t s_current_zcr = 0;         /* 当前过零率 */
static volatile uint8_t s_detection_count = 0;      /* 连续检测计数 */

/* 检测阈值 */
static uint32_t s_energy_threshold = AUDIO_ENERGY_THRESHOLD;
static uint16_t s_zcr_threshold = AUDIO_ZCR_THRESHOLD;

/* 回调函数 */
static AudioDetectCallback_t s_detect_callback = NULL;

/* 初始化标志 */
static volatile uint8_t s_initialized = 0;

/* FreeRTOS任务句柄 */
TaskHandle_t g_audio_monitor_task_handle = NULL;

/* TinyML推理任务句柄 (在main.c中定义并赋值) */
extern TaskHandle_t g_tinyml_task_handle;

/* 上次通知推理任务时的写入量, 用于按1秒间隔触发 */
static uint32_t s_last_notify_total = 0;

/***************************************************************************************
 *                                    私有函数
 ***************************************************************************************/

/**
 * @brief       I2S RX DMA回调函数
 * @note        在DMA传输完成中断中调用
 */
static void audio_monitor_dma_callback(void)
{
    /* 切换缓冲区索引 */
    if (I2S_RX_DMASx->CR & DMA_SxCR_CT)
    {
        s_current_buf = 1;  /* DMA正在使用buf1，buf0可以处理 */
    }
    else
    {
        s_current_buf = 0;  /* DMA正在使用buf0，buf1可以处理 */
    }
    
    s_buf_ready = 1;
    
    /* 通知任务有新数据 */
    if (g_audio_monitor_task_handle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(g_audio_monitor_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief       从立体声数据提取单声道
 * @param       stereo : 立体声数据 (L, R, L, R, ...)
 * @param       mono   : 单声道输出
 * @param       samples: 采样点数
 */
static void extract_mono(const int16_t *stereo, int16_t *mono, uint16_t samples)
{
    for (uint16_t i = 0; i < samples; i++)
    {
        /* 取左右声道平均值 */
        int32_t sum = (int32_t)stereo[i * 2] + (int32_t)stereo[i * 2 + 1];
        mono[i] = (int16_t)(sum / 2);
    }
}

/**
 * @brief       计算音频帧能量
 * @param       buf    : 音频数据
 * @param       samples: 采样点数
 * @return      能量值 (均方根的平方，除以采样数)
 */
static uint32_t calculate_energy(const int16_t *buf, uint16_t samples)
{
    uint64_t sum = 0;
    
    for (uint16_t i = 0; i < samples; i++)
    {
        int32_t val = (int32_t)buf[i];
        sum += (uint64_t)(val * val);
    }
    
    return (uint32_t)(sum / samples);
}

/**
 * @brief       计算过零率 (Zero Crossing Rate)
 * @param       buf    : 音频数据
 * @param       samples: 采样点数
 * @return      过零次数
 * @note        高频信号的过零率较高
 */
static uint16_t calculate_zcr(const int16_t *buf, uint16_t samples)
{
    uint16_t zcr = 0;
    
    for (uint16_t i = 1; i < samples; i++)
    {
        /* 检测符号变化 */
        if ((buf[i] >= 0 && buf[i-1] < 0) || (buf[i] < 0 && buf[i-1] >= 0))
        {
            zcr++;
        }
    }
    
    return zcr;
}

/**
 * @brief       处理音频缓冲区
 * @param       buf    : 立体声音频数据
 */
static void process_audio_buffer(const int16_t *buf)
{
    static uint32_t s_dbg_count = 0;
    
    /* 提取单声道 */
    extract_mono(buf, s_process_buf, AUDIO_MONITOR_BUF_SIZE);
    
    /* 计算能量 */
    s_current_energy = calculate_energy(s_process_buf, AUDIO_MONITOR_BUF_SIZE);
    
    /* 计算过零率 */
    s_current_zcr = calculate_zcr(s_process_buf, AUDIO_MONITOR_BUF_SIZE);
    
    /* 每100帧(约3.2秒@16kHz/512)打印一次调试信息 */
    if (++s_dbg_count % 100 == 1)
    {
        printf("[AudioMonitor] frame#%lu energy=%lu zcr=%u\r\n",
               s_dbg_count, (unsigned long)s_current_energy, s_current_zcr);
    }
    
    /* 判断是否检测到高频声音 */
    uint8_t frame_detected = 0;
    
    if (s_current_energy > s_energy_threshold && s_current_zcr > s_zcr_threshold)
    {
        frame_detected = 1;
    }
    
    /* 连续检测逻辑 */
    if (frame_detected)
    {
        s_detection_count++;
        if (s_detection_count >= AUDIO_DETECTION_COUNT)
        {
            if (!s_detected)  /* 首次检测到 */
            {
                s_detected = 1;
                
                /* 调用回调函数 */
                if (s_detect_callback != NULL)
                {
                    s_detect_callback(1, s_current_energy, s_current_zcr);
                }
            }
        }
    }
    else
    {
        if (s_detection_count > 0)
        {
            s_detection_count--;
        }
        
        if (s_detection_count == 0 && s_detected)
        {
            s_detected = 0;
            
            /* 调用回调函数 */
            if (s_detect_callback != NULL)
            {
                s_detect_callback(0, s_current_energy, s_current_zcr);
            }
        }
    }
    
    /* === TinyML 集成: 写入环形缓冲 + 触发推理 === */
    audio_ringbuf_write(s_process_buf, AUDIO_MONITOR_BUF_SIZE);
    
    /* 每积累1秒(RINGBUF_SIZE样本)通知推理任务 */
    uint32_t total = audio_ringbuf_get_total_written();
    if (audio_ringbuf_is_ready() &&
        (total - s_last_notify_total) >= RINGBUF_SIZE)
    {
        s_last_notify_total = total;
        if (g_tinyml_task_handle != NULL)
        {
            xTaskNotifyGive(g_tinyml_task_handle);
        }
    }
}

/***************************************************************************************
 *                                    公共函数
 ***************************************************************************************/

/**
 * @brief       初始化音频监听模块
 */
void audio_monitor_init(void)
{
    /* 清零缓冲区 */
    memset(s_rx_buf0, 0, sizeof(s_rx_buf0));
    memset(s_rx_buf1, 0, sizeof(s_rx_buf1));
    memset(s_process_buf, 0, sizeof(s_process_buf));
    
    /* 复位状态 */
    s_monitor_mode = AUDIO_MONITOR_MODE_OFF;
    s_buf_ready = 0;
    s_current_buf = 0;
    s_detected = 0;
    s_current_energy = 0;
    s_current_zcr = 0;
    s_detection_count = 0;
    
    /* 设置DMA回调 */
    i2s_rx_callback = audio_monitor_dma_callback;
    
    /* 初始化RX DMA */
    i2s_rx_dma_init((uint8_t *)s_rx_buf0, (uint8_t *)s_rx_buf1, 
                    AUDIO_MONITOR_BUF_SIZE * AUDIO_MONITOR_CHANNELS);
    
    s_initialized = 1;
    printf("[AudioMonitor] init OK (DMA+callback configured)\r\n");
}

/**
 * @brief       开始音频监听
 * @param       mode : 监听模式
 */
void audio_monitor_start(AudioMonitorMode_t mode)
{
    if (mode == AUDIO_MONITOR_MODE_OFF)
    {
        audio_monitor_stop();
        return;
    }
    
    /* 确保DMA和回调已初始化 (解决任务优先级导致的竞态条件) */
    if (!s_initialized)
    {
        audio_monitor_init();
    }
    
    s_monitor_mode = mode;
    printf("[AudioMonitor] start mode=%d\r\n", (int)mode);
    
    /* 配置ES8388启用ADC */
    if (mode == AUDIO_MONITOR_MODE_ONLY)
    {
        /* 仅监听模式：关闭DAC，开启ADC */
        es8388_adda_cfg(0, 1);
    }
    else /* AUDIO_MONITOR_MODE_BACKGROUND */
    {
        /* 后台监听模式：同时开启DAC和ADC */
        es8388_adda_cfg(1, 1);
    }
    
    /* 配置ADC输入通道 (使用MIC输入) */
    es8388_input_cfg(0);  /* 通道1输入 */
    es8388_mic_gain(4);   /* 设置MIC增益 12dB */
    
    /* 初始化全双工模式 */
    i2s_fullduplex_init(AUDIO_MONITOR_SAMPLE_RATE);
    
    /* 开始接收 */
    i2s_record_start();
    
    /* 如果是后台模式，同时开始播放 */
    if (mode == AUDIO_MONITOR_MODE_BACKGROUND)
    {
        i2s_play_start();
    }
}

/**
 * @brief       停止音频监听
 */
void audio_monitor_stop(void)
{
    /* 停止接收 */
    i2s_record_stop();
    
    /* 复位状态 */
    s_monitor_mode = AUDIO_MONITOR_MODE_OFF;
    s_buf_ready = 0;
    s_detected = 0;
    s_detection_count = 0;
    
    /* 恢复ES8388为仅DAC模式 */
    es8388_adda_cfg(1, 0);
}

/**
 * @brief       设置检测回调函数
 * @param       callback : 回调函数
 */
void audio_monitor_set_callback(AudioDetectCallback_t callback)
{
    s_detect_callback = callback;
}

/**
 * @brief       设置检测阈值
 * @param       energy_th : 能量阈值
 * @param       zcr_th    : 过零率阈值
 */
void audio_monitor_set_threshold(uint32_t energy_th, uint16_t zcr_th)
{
    s_energy_threshold = energy_th;
    s_zcr_threshold = zcr_th;
}

/**
 * @brief       获取当前监听模式
 * @return      当前模式
 */
AudioMonitorMode_t audio_monitor_get_mode(void)
{
    return s_monitor_mode;
}

/**
 * @brief       是否检测到高频声音
 * @return      1=检测到, 0=未检测到
 */
uint8_t audio_monitor_is_detected(void)
{
    return s_detected;
}

/**
 * @brief       获取当前能量值
 * @return      能量值
 */
uint32_t audio_monitor_get_energy(void)
{
    return s_current_energy;
}

/**
 * @brief       获取当前过零率
 * @return      过零率
 */
uint16_t audio_monitor_get_zcr(void)
{
    return s_current_zcr;
}

/**
 * @brief       音频监听FreeRTOS任务
 * @param       pvParameters : 任务参数 (未使用)
 */
void vAudioMonitor_Task(void *pvParameters)
{
    (void)pvParameters;
    
     /* 初始化监听模块 (如果audio_monitor_start还没调用过init) */
    if (!s_initialized)
    {
        audio_monitor_init();
    }
    else
    {
        printf("[AudioMonitor] task: init already done by start()\r\n");
    }
    
    while (1)
    {
        /* 等待DMA通知 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* 检查是否有缓冲区就绪 */
        if (s_buf_ready && s_monitor_mode != AUDIO_MONITOR_MODE_OFF)
        {
            s_buf_ready = 0;
            
            /* 处理已经采集完成的缓冲区 */
            if (s_current_buf == 1)
            {
                process_audio_buffer(s_rx_buf0);
            }
            else
            {
                process_audio_buffer(s_rx_buf1);
            }
        }
    }
}
