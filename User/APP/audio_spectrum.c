/**
 ****************************************************************************************************
 * @file        audio_spectrum.c
 * @brief       音频实时频谱分析模块
 * @details     使用 CMSIS-DSP arm_rfft_fast_f32 对播放PCM做512点实数FFT,
 *              将257个频率bin按对数间隔合并为15个柱状频段 (类似EQ可视化)
 *
 *              优化: 全程使用功率谱(re²+im²), 避免逐bin sqrtf
 *              映射: 只在最终15个频段上用两次sqrtf做四次方根压缩 (替代昂贵的log10f)
 *              平滑: 上升EMA快速跟踪 + 下降自然衰减
 ****************************************************************************************************
 */

#include "audio_spectrum.h"
#include "arm_math.h"
#include "FreeRTOS.h"
#include <string.h>

/* ======================== 配置 ======================== */
#define FFT_LEN         512         /* FFT点数 */
#define HALF_FFT        (FFT_LEN / 2)      /* 256 */
#define NUM_BINS        (FFT_LEN / 2 + 1)  /* 257个频率bin */
#define SMOOTH_UP       0.55f       /* 上升EMA系数 */
#define SMOOTH_DOWN     0.80f       /* 下降衰减系数 */

/* ======================== 私有变量 ======================== */
static arm_rfft_fast_instance_f32 s_rfft;
static float32_t *s_fft_in  = NULL;    /* [FFT_LEN] */
static float32_t *s_fft_out = NULL;    /* [FFT_LEN] */
static float32_t *s_psd     = NULL;    /* [NUM_BINS] 功率谱 */
static float32_t  s_smooth[SPECTRUM_NUM_BARS] = {0};
static uint8_t    s_inited = 0;

static const uint16_t s_band_edges[SPECTRUM_NUM_BARS + 1] = {
 /* 86   172  258  344  516  688  946 1290 1720 2408 3268 4472 6192 8614 12006  22050 Hz */
    1,   2,   3,   4,   6,   8,  11,  15,  20,  28,  38,  52,  72, 100,  140,  257
};

/* (N/2)² 的倒数, 用于归一化功率谱 */
#define INV_HALF_N_SQ   (1.0f / ((float32_t)HALF_FFT * (float32_t)HALF_FFT))

/* ======================== 公有接口 ======================== */

int audio_spectrum_init(void)
{
    if (s_inited) return 0;

    s_fft_in  = pvPortMalloc(FFT_LEN * sizeof(float32_t));
    s_fft_out = pvPortMalloc(FFT_LEN * sizeof(float32_t));
    s_psd     = pvPortMalloc(NUM_BINS * sizeof(float32_t));

    if (!s_fft_in || !s_fft_out || !s_psd) return -1;

    arm_rfft_fast_init_f32(&s_rfft, FFT_LEN);

    memset(s_smooth, 0, sizeof(s_smooth));
    s_inited = 1;
    return 0;
}
//sd得到的 未传到es8388 的buf, 字节数，位数（16/24）,返回频谱柱状图数据缓冲区
void audio_spectrum_calc(const uint8_t *pcm_buf, uint32_t buf_size,
                         uint8_t bps, uint8_t *bars)
{
    if (!s_inited || !pcm_buf || !bars) {
        memset(bars, 0, SPECTRUM_NUM_BARS);
        return;
    }

    /* ===== 1) 从PCM提取单声道float样本 ===== */
    uint32_t samples_needed = FFT_LEN;
    
    if (bps == 16) {
        const int16_t *p16 = (const int16_t *)pcm_buf;
        uint32_t stereo_samples = buf_size / 4;  /* 每个立体声样本4字节 */
        if (stereo_samples < samples_needed) samples_needed = stereo_samples;

        for (uint32_t i = 0; i < samples_needed; i++) {
            /* 取左声道, 归一化到 [-1, 1) */
            s_fft_in[i] = (float32_t)p16[i * 2] / 32768.0f;
        }
    } else if (bps == 24) {
        /* 24bit在wavplay中已转为32bit格式: 每样本4字节, 立体声8字节 */
        const int32_t *p32 = (const int32_t *)pcm_buf;
        uint32_t stereo_samples = buf_size / 8;
        if (stereo_samples < samples_needed) samples_needed = stereo_samples;

        for (uint32_t i = 0; i < samples_needed; i++) {
            s_fft_in[i] = (float32_t)p32[i * 2] / 2147483648.0f;
        }
    } else {
        memset(bars, 0, SPECTRUM_NUM_BARS);
        return;
    }

    /* 不足FFT_LEN的部分补零 */
    if (samples_needed < FFT_LEN) {
        arm_fill_f32(0.0f, &s_  fft_in[samples_needed], FFT_LEN - samples_needed);
    }

    /* ===== 2) 实数FFT ===== */
    arm_rfft_fast_f32(&s_rfft, s_fft_in, s_fft_out, 0);

    /* ===== 3) 功率谱 (re² + im²), 不取sqrt ===== */
    s_psd[0]            = s_fft_out[0] * s_fft_out[0];   /* DC */
    s_psd[NUM_BINS - 1] = s_fft_out[1] * s_fft_out[1];   /* Nyquist */

    for (uint32_t i = 1; i < NUM_BINS - 1; i++) {
        float32_t re = s_fft_out[2 * i];
        float32_t im = s_fft_out[2 * i + 1];
        s_psd[i] = re * re + im * im;
    }

    /* ===== 4) 合并到15个频段, 四次方根映射 ===== */
    for (uint32_t b = 0; b < SPECTRUM_NUM_BARS; b++) {
        uint16_t start = s_band_edges[b];//低频映射 bins少，高频映射 bins多，符合人耳对数频率感知特性
        uint16_t end   = s_band_edges[b + 1];
        float32_t sum_p = 0.0f;

        for (uint16_t k = start; k < end; k++) {
            sum_p += s_psd[k];
        }

        /* 平均功率, 归一化: 对正弦满幅信号 power = (N/2)² */
        float32_t avg_p = sum_p / (float32_t)(end - start) * INV_HALF_N_SQ;

        /* 四次方根压缩: power^0.25, 类似dB感知但无需log10f
         * 增益180: -6dB→127(clip100), -12dB→90, -18dB→64 */
        float32_t val = sqrtf(sqrtf(avg_p)) * 180.0f;
        if (val > 100.0f) val = 100.0f;

        /* 非对称EMA: 上升快跟踪, 下降慢衰减 */
        if (val >= s_smooth[b]) {
            s_smooth[b] = SMOOTH_UP * val + (1.0f - SMOOTH_UP) * s_smooth[b];
        } else {
            s_smooth[b] *= SMOOTH_DOWN;
            if (s_smooth[b] < val) s_smooth[b] = val;
        }

        bars[b] = (uint8_t)(s_smooth[b] + 0.5f);
    }
}
