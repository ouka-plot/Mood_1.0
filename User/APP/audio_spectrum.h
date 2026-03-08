/**
 ****************************************************************************************************
 * @file        audio_spectrum.h
 * @brief       音频实时频谱分析模块
 * @details     对播放中的PCM数据做512点FFT, 将257个频率bin合并为15个柱状频段,
 *              输出0~100的显示值供LVGL chart使用
 ****************************************************************************************************
 */

#ifndef __AUDIO_SPECTRUM_H
#define __AUDIO_SPECTRUM_H

#include <stdint.h>

#define SPECTRUM_NUM_BARS    15      /* 频谱柱数 (与chart的point_count一致) */

/**
 * @brief  初始化频谱分析模块
 * @note   分配FFT缓冲区 (从CCM heap)
 * @retval 0=成功, -1=内存分配失败
 */
int audio_spectrum_init(void);

/**
 * @brief  从PCM数据计算15段频谱
 * @param  pcm_buf : 立体声16位PCM数据 (L,R,L,R...)
 * @param  samples : 总采样数 (含L+R, 即立体声样本数×2)
 * @param  bps     : 位深 (16或24)
 * @param  bars    : 输出频谱柱值 [SPECTRUM_NUM_BARS], 范围0~100
 */
void audio_spectrum_calc(const uint8_t *pcm_buf, uint32_t buf_size,
                         uint8_t bps, uint8_t *bars);

#endif /* __AUDIO_SPECTRUM_H */
