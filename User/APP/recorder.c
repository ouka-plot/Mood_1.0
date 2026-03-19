/**
 ****************************************************************************************************
 * @file        recorder.c
 * @brief       WAV录音模块实现 - 通过I2S RX采集音频并保存为WAV文件到SD卡
 * 
 * @note        录音流程：
 *              1. 配置ES8388开启ADC，配置I2S全双工(TX静音+RX录音)
 *              2. DMA双缓冲接收，中断中设置就绪标志
 *              3. 任务中将DMA缓冲区数据写入SD卡
 *              4. 停止时回填WAV文件头(数据大小)
 ****************************************************************************************************
 */

#include "./APP/recorder.h"
#include "./BSP/I2S/i2s.h"
#include "./BSP/ES8388/es8388.h"
#include "./BSP/LED/led.h"
#include "./FATFS/source/ff.h"
#include "./SYSTEM/USART/usart.h"
#include "./AUDIOCODEC/wav/wavplay.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

#define RECORDER_SOFT_GAIN   8
#define RECORDER_NOISE_FLOOR_LOW   80
#define RECORDER_NOISE_FLOOR_HIGH  160

/***************************************************************************************
 *                                    私有变量
 ***************************************************************************************/

/* DMA双缓冲区 */
static int16_t s_rec_buf0[RECORDER_BUF_SIZE * RECORDER_CHANNELS];
static int16_t s_rec_buf1[RECORDER_BUF_SIZE * RECORDER_CHANNELS];

/* WAV文件对象 */
static FIL s_rec_file;

/* 状态 */
static volatile RecorderState_t s_rec_state = RECORDER_STATE_IDLE;
static volatile uint8_t s_buf_ready = 0;         /* 缓冲区就绪标志 */
static volatile uint8_t s_current_buf = 0;       /* 当前已完成的缓冲区编号 */
static uint32_t s_data_size = 0;                 /* 已录制的数据大小(字节) */
static uint32_t s_start_tick = 0;                /* 录音开始时的tick */
static uint32_t s_paused_duration = 0;           /* 暂停累计时长(tick) */
static uint32_t s_pause_start_tick = 0;          /* 本次暂停开始时的tick */
static uint32_t s_final_duration = 0;            /* 停止时的最终录音时长(秒) */

/* 文件名缓冲区 */
static char s_rec_fname[64];

/* LED闪烁控制 */
static uint32_t s_led_counter = 0;

static int16_t recorder_apply_gain(int16_t sample)
{
    int32_t scaled = (int32_t)sample * RECORDER_SOFT_GAIN;

    if (scaled > 32767)
    {
        return 32767;
    }

    if (scaled < -32768)
    {
        return -32768;
    }

    return (int16_t)scaled;
}

static int16_t recorder_soft_denoise(int16_t sample)
{
    int32_t abs_sample = (sample >= 0) ? sample : -sample;

    if (abs_sample < RECORDER_NOISE_FLOOR_LOW)
    {
        return (int16_t)(sample / 4);
    }

    if (abs_sample < RECORDER_NOISE_FLOOR_HIGH)
    {
        return (int16_t)(sample / 2);
    }

    return sample;
}

/***************************************************************************************
 *                                    私有函数
 ***************************************************************************************/

/**
 * @brief       生成录音文件名 (REC_XXXX.WAV)
 */
static void recorder_gen_filename(void)
{
    DIR dir;
    FILINFO finfo;
    uint16_t max_idx = 0;
    uint16_t idx;
    
    /* 扫描目录找最大编号 */
    if (f_opendir(&dir, RECORDER_SAVE_DIR) == FR_OK)
    {
        while (1)
        {
            if (f_readdir(&dir, &finfo) != FR_OK || finfo.fname[0] == 0)
                break;
            /* 匹配 REC_XXXX.WAV 格式 */
            if (strncmp(finfo.fname, "REC_", 4) == 0)
            {
                idx = 0;
                for (int i = 4; i < 8 && finfo.fname[i] >= '0' && finfo.fname[i] <= '9'; i++)
                {
                    idx = idx * 10 + (finfo.fname[i] - '0');
                }
                if (idx >= max_idx)
                    max_idx = idx + 1;
            }
        }
        f_closedir(&dir);
    }
    
    snprintf(s_rec_fname, sizeof(s_rec_fname), "%s/REC_%04u.WAV", RECORDER_SAVE_DIR, max_idx);
}

/**
 * @brief       写入WAV文件头(占位，停止时回填)
 */
static uint8_t recorder_write_header(void)
{
    __WaveHeader header;
    UINT bw;
    
    memset(&header, 0, sizeof(header));
    
    /* RIFF chunk */
    header.riff.ChunkID = 0x46464952;   /* "RIFF" */
    header.riff.ChunkSize = 0;          /* 占位, 停止时回填 */
    header.riff.Format = 0x45564157;     /* "WAVE" */
    
    /* fmt chunk */
    header.fmt.ChunkID = 0x20746D66;     /* "fmt " */
    header.fmt.ChunkSize = 16;           /* PCM固定16字节 */
    header.fmt.AudioFormat = 1;          /* PCM */
    header.fmt.NumOfChannels = RECORDER_CHANNELS;
    header.fmt.SampleRate = RECORDER_SAMPLE_RATE;
    header.fmt.BitsPerSample = RECORDER_BITS;
    header.fmt.BlockAlign = RECORDER_CHANNELS * RECORDER_BITS / 8;
    header.fmt.ByteRate = RECORDER_SAMPLE_RATE * header.fmt.BlockAlign;
    
    /* data chunk */
    header.data.ChunkID = 0x61746164;    /* "data" */
    header.data.ChunkSize = 0;           /* 占位, 停止时回填 */
    
    return f_write(&s_rec_file, &header, sizeof(header), &bw);
}

/**
 * @brief       回填WAV文件头(更新数据大小)
 */
static uint8_t recorder_update_header(void)
{
    uint32_t temp;
    UINT bw;
    
    /* 回到文件头, 更新RIFF ChunkSize */
    f_lseek(&s_rec_file, 4);
    temp = s_data_size + 36;   /* 文件总大小 - 8 */
    f_write(&s_rec_file, &temp, 4, &bw);
    
    /* 更新data ChunkSize */
    f_lseek(&s_rec_file, 40);
    f_write(&s_rec_file, &s_data_size, 4, &bw);
    
    return 0;
}

/***************************************************************************************
 *                                    公共函数
 ***************************************************************************************/

void recorder_init(void)
{
    FRESULT res;
    
    /* 创建录音目录(如果不存在) */
    res = f_mkdir(RECORDER_SAVE_DIR);
    if (res == FR_OK || res == FR_EXIST)
    {
        printf("[Recorder] Directory %s ready\r\n", RECORDER_SAVE_DIR);
    }
    else
    {
        printf("[Recorder] Failed to create dir: %d\r\n", res);
    }
    
    s_rec_state = RECORDER_STATE_IDLE;
    s_data_size = 0;
}

uint8_t recorder_start(void)
{
    FRESULT res;
    
    if (s_rec_state == RECORDER_STATE_RECORDING)
        return 1;  /* 已在录音 */
    
    /* 生成文件名 */
    recorder_gen_filename();
    printf("[Recorder] Start recording: %s\r\n", s_rec_fname);
    
    /* 创建文件 */
    res = f_open(&s_rec_file, s_rec_fname, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("[Recorder] Failed to create file: %d\r\n", res);
        return 2;
    }
    
    /* 写入WAV头(占位) */
    recorder_write_header();
    s_data_size = 0;
    
    /* 配置ES8388: 开启ADC */
    es8388_adda_cfg(1, 1);          /* DAC开, ADC开 */
    es8388_write_reg(0x03, 0x00);   /* ADC电源全部上电(模拟输入+ADC+MIC偏置) */
    es8388_input_cfg(0);            /* MIC1输入 (LINPUT1/RINPUT1) */
    es8388_mic_gain(8);             /* MIC增益 24dB (8*3dB) */
    es8388_output_cfg(1, 0);        /* 保持耳机输出(可以监听) */
    
    /* 配置I2S: 16位, 设置采样率 */
    es8388_i2s_cfg(0, 3);           /* 飞利浦标准, 16位 */
    i2s_init(I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_16B_EXTENDED);
    i2s_samplerate_set(RECORDER_SAMPLE_RATE);

    /* 初始化I2S2ext(全双工接收) */
    i2s2ext_init();
    
    /* 配置RX DMA双缓冲 */
    i2s_rx_dma_init((uint8_t*)s_rec_buf0, (uint8_t*)s_rec_buf1, 
                    RECORDER_BUF_SIZE * RECORDER_CHANNELS);
    i2s_rx_callback = recorder_dma_callback;
    
    /* 配置TX DMA (输出静音, 但TX必须运行以驱动时钟) */
    {
        static int16_t s_silence_buf[64];
        memset(s_silence_buf, 0, sizeof(s_silence_buf));
        i2s_tx_dma_init((uint8_t*)s_silence_buf, (uint8_t*)s_silence_buf, 32);
    }
    
    s_buf_ready = 0;
    s_start_tick = xTaskGetTickCount();
    s_paused_duration = 0;
    s_led_counter = 0;
    
    /* 启动DMA */
    i2s_play_start();    /* TX必须启动以产生时钟 */
    i2s_record_start();  /* RX开始接收 */
    
    s_rec_state = RECORDER_STATE_RECORDING;
    LED0(0);  /* 点亮LED0表示录音中 */
    
    return 0;
}

void recorder_pause(void)
{
    if (s_rec_state != RECORDER_STATE_RECORDING)
        return;
    
    s_rec_state = RECORDER_STATE_PAUSED;
    s_pause_start_tick = xTaskGetTickCount();
    /* 不停止DMA, 只是不写入数据 */
    printf("[Recorder] Paused\r\n");
}

void recorder_resume(void)
{
    if (s_rec_state != RECORDER_STATE_PAUSED)
        return;
    
    /* 累加暂停时长 */
    s_paused_duration += xTaskGetTickCount() - s_pause_start_tick;
    s_rec_state = RECORDER_STATE_RECORDING;
    printf("[Recorder] Resumed\r\n");
}

uint8_t recorder_stop(void)
{
    if (s_rec_state != RECORDER_STATE_RECORDING && 
        s_rec_state != RECORDER_STATE_PAUSED)
        return 1;

    s_rec_state = RECORDER_STATE_SAVING;

    /* 缓存最终时长 */
    s_final_duration = recorder_get_duration();
    
    /* 停止DMA */
    i2s_record_stop();
    i2s_play_stop();
    
    /* 回填WAV文件头 */
    recorder_update_header();
    
    /* 同步并关闭文件 */
    f_sync(&s_rec_file);
    f_close(&s_rec_file);
    
    LED0(1);  /* 熄灭LED0 */
    
    printf("[Recorder] Saved: %s (%lu bytes, %lu sec)\r\n", 
           s_rec_fname, s_data_size, recorder_get_duration());
    
    /* 恢复ES8388为播放模式 */
    es8388_write_reg(0x03, 0x09);   /* 关闭ADC电源和MIC偏置 */
    es8388_adda_cfg(1, 0);      /* 开启DAC关闭ADC */
    es8388_output_cfg(1, 0);    /* OUT1(耳机)开 */
    
    s_rec_state = RECORDER_STATE_DONE;
    return 0;
}

void recorder_abort(void)
{
    if (s_rec_state == RECORDER_STATE_IDLE || s_rec_state == RECORDER_STATE_DONE)
        return;
    
    /* 停止DMA */
    i2s_record_stop();
    i2s_play_stop();
    
    /* 关闭并删除文件 */
    f_close(&s_rec_file);
    f_unlink(s_rec_fname);
    
    LED0(1);
    
    /* 恢复ES8388为播放模式 */
    es8388_write_reg(0x03, 0x09);   /* 关闭ADC电源和MIC偏置 */
    es8388_adda_cfg(1, 0);
    es8388_output_cfg(1, 0);
    
    s_rec_state = RECORDER_STATE_IDLE;
    printf("[Recorder] Aborted\r\n");
}

RecorderState_t recorder_get_state(void)
{
    return s_rec_state;
}

uint32_t recorder_get_duration(void)
{
    if (s_rec_state == RECORDER_STATE_IDLE)
        return 0;
    if (s_rec_state == RECORDER_STATE_DONE || s_rec_state == RECORDER_STATE_SAVING)
        return s_final_duration;
    
    uint32_t elapsed = xTaskGetTickCount() - s_start_tick - s_paused_duration;
    if (s_rec_state == RECORDER_STATE_PAUSED)
        elapsed -= (xTaskGetTickCount() - s_pause_start_tick);
    
    return elapsed / configTICK_RATE_HZ;  /* 转换为秒 */
}

uint32_t recorder_get_data_size(void)
{
    return s_data_size;
}

/* DMA回调(ISR上下文) */
void recorder_dma_callback(void)
{
    if (I2S_RX_DMASx->CR & DMA_SxCR_CT)
    {
        s_current_buf = 0;  /* DMA切到buf1，buf0可处理 */
    }
    else
    {
        s_current_buf = 1;  /* DMA切到buf0，buf1可处理 */
    }
    s_buf_ready = 1;
}

/**
 * @brief       录音处理函数(在任务循环中调用)
 * @note        将DMA缓冲区数据写入SD卡, LED闪烁指示
 */
void recorder_process(void)
{
    UINT bw;
    int16_t *src;
    uint32_t write_size;
    
    if (s_rec_state != RECORDER_STATE_RECORDING && 
        s_rec_state != RECORDER_STATE_PAUSED)
        return;
    
    if (!s_buf_ready)
        return;
    
    s_buf_ready = 0;
    
    /* 选择已完成的缓冲区 */
    src = (s_current_buf == 0) ? s_rec_buf0 : s_rec_buf1;
    write_size = RECORDER_BUF_SIZE * RECORDER_CHANNELS * sizeof(int16_t);
    
    /* 仅录音状态才写入, 暂停时丢弃数据 */
    if (s_rec_state == RECORDER_STATE_RECORDING)
    {
        /* 检查是否超过最大时长 */
        if (recorder_get_duration() >= RECORDER_MAX_DURATION)
        {
            recorder_stop();
            return;
        }

        for (uint16_t i = 0; i < (RECORDER_BUF_SIZE * RECORDER_CHANNELS); i++)
        {
            src[i] = recorder_soft_denoise(recorder_apply_gain(src[i]));
        }

        FRESULT res = f_write(&s_rec_file, src, write_size, &bw);
        if (res == FR_OK && bw == write_size)
        {
            s_data_size += write_size;
        }
        else
        {
            printf("[Recorder] Write error: %d\r\n", res);
            recorder_stop();
            return;
        }
        
        /* 每秒同步一次避免丢数据 */
        if ((s_data_size % (RECORDER_SAMPLE_RATE * RECORDER_CHANNELS * sizeof(int16_t))) < write_size)
        {
            f_sync(&s_rec_file);
        }
    }
    
    /* LED闪烁 - 录音时快闪, 暂停时慢闪 */
    s_led_counter++;
    if (s_rec_state == RECORDER_STATE_RECORDING)
    {
        if (s_led_counter % 16 == 0)  /* 约0.5秒闪一次 */
            LED0_TOGGLE();
    }
    else if (s_rec_state == RECORDER_STATE_PAUSED)
    {
        if (s_led_counter % 48 == 0)  /* 约1.5秒闪一次 */
            LED0_TOGGLE();
    }
}
