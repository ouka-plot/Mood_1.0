/**
 ****************************************************************************************************
 * @file        audio_features.c
 * @brief       MFCC 特征提取模块 - 基于 CMSIS-DSP arm_mfcc_f32
 * @details     初始化时计算 Hamming窗 + Mel滤波器组 + DCT矩阵，
 *              每帧调用 arm_mfcc_f32 完成 窗函数→FFT→Mel→log→DCT 流水线
 ****************************************************************************************************
 */

#include "audio_features.h"
#include "dsp/transform_functions.h"
#include "FreeRTOS.h"
#include <math.h>

/* ======================== 私有变量 ======================== */
static arm_mfcc_instance_f32 s_mfcc_inst;
static uint8_t s_initialized = 0;

/* 系数数组 (init时从CCM heap分配) */
static float32_t *s_hamming_window;         /* [MFCC_FFT_LEN] */
static float32_t *s_dct_matrix;             /* [MFCC_NUM_COEFFS * MFCC_NUM_MEL_FILTERS] */
static float32_t *s_mel_coefs;              /* 稀疏三角滤波器系数 (可变长) */
static uint32_t  *s_mel_filter_pos;         /* [MFCC_NUM_MEL_FILTERS] 滤波器起始FFT bin */
static uint32_t  *s_mel_filter_lengths;     /* [MFCC_NUM_MEL_FILTERS] 滤波器长度 */

/* 工作缓冲 (init时从CCM heap分配) */
static float32_t *s_frame_buf;              /* [MFCC_FFT_LEN] 帧缓冲(被arm_mfcc_f32修改) */
static float32_t *s_tmp_buf;               /* [MFCC_FFT_LEN + 2] FFT临时缓冲 */

/* ======================== Mel频率转换 ======================== */

static float32_t hz_to_mel(float32_t hz)
{
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}

static float32_t mel_to_hz(float32_t mel)
{
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

/* ======================== 系数计算 ======================== */

/**
 * @brief  计算Hamming窗系数
 *         w[n] = 0.54 - 0.46 * cos(2π·n / (N-1))
 */
static void compute_hamming(float32_t *window, uint32_t len)
{
    float32_t denom = (float32_t)(len - 1);
    for (uint32_t i = 0; i < len; i++) {
        window[i] = 0.54f - 0.46f * cosf(2.0f * PI * (float32_t)i / denom);
    }
}

/**
 * @brief  计算DCT-II正交矩阵
 *         D[k][n] = sqrt(2/N) * cos(π·k·(n+0.5)/N),  k>0
 *         D[0][n] = 1/sqrt(N)
 */
static void compute_dct_matrix(float32_t *dct, uint32_t n_out, uint32_t n_mel)
{
    float32_t norm0 = 1.0f / sqrtf((float32_t)n_mel);
    float32_t norm  = sqrtf(2.0f / (float32_t)n_mel);

    for (uint32_t n = 0; n < n_mel; n++) {
        dct[n] = norm0;
    }
    for (uint32_t k = 1; k < n_out; k++) {
        for (uint32_t n = 0; n < n_mel; n++) {
            dct[k * n_mel + n] = norm * cosf(
                PI * (float32_t)k * ((float32_t)n + 0.5f) / (float32_t)n_mel
            );
        }
    }
}

/**
 * @brief  计算Mel三角滤波器组 (稀疏表示)
 * @note   生成CMSIS-DSP要求的filterPos/filterLengths/filterCoefs三元组
 *         每个滤波器是三角形: 从左边频→中心频→右边频
 */
static int compute_mel_filters(void)
{
    uint32_t n_fft_bins = MFCC_FFT_LEN / 2 + 1;   /* 257 */
    float32_t freq_res = (float32_t)MFCC_SAMPLE_RATE / (float32_t)MFCC_FFT_LEN;  /* 31.25 Hz */

    /* 在Mel尺度上等间距放置 n_mels+2 个点 */
    float32_t mel_min = hz_to_mel(MFCC_FREQ_MIN);
    float32_t mel_max = hz_to_mel(MFCC_FREQ_MAX);
    float32_t mel_step = (mel_max - mel_min) / (float32_t)(MFCC_NUM_MEL_FILTERS + 1);

    /* 转换为FFT bin索引 */
    uint32_t bins[MFCC_NUM_MEL_FILTERS + 2];
    for (uint32_t i = 0; i < MFCC_NUM_MEL_FILTERS + 2; i++) {
        float32_t mel = mel_min + (float32_t)i * mel_step;
        float32_t hz  = mel_to_hz(mel);
        uint32_t bin  = (uint32_t)(hz / freq_res + 0.5f);
        if (bin >= n_fft_bins) bin = n_fft_bins - 1;
        bins[i] = bin;
    }

    /* 第一遍: 统计总系数个数 */
    uint32_t total_coefs = 0;
    for (uint32_t m = 0; m < MFCC_NUM_MEL_FILTERS; m++) {
        uint32_t left  = bins[m];
        uint32_t right = bins[m + 2];
        total_coefs += (right > left) ? (right - left + 1) : 1;
    }

    /* 分配数组 (CCM heap) */
    s_mel_filter_pos     = pvPortMalloc(MFCC_NUM_MEL_FILTERS * sizeof(uint32_t));
    s_mel_filter_lengths = pvPortMalloc(MFCC_NUM_MEL_FILTERS * sizeof(uint32_t));
    s_mel_coefs          = pvPortMalloc(total_coefs * sizeof(float32_t));

    if (!s_mel_filter_pos || !s_mel_filter_lengths || !s_mel_coefs) {
        return -1;
    }

    /* 第二遍: 填充三角滤波器系数 */
    uint32_t idx = 0;
    for (uint32_t m = 0; m < MFCC_NUM_MEL_FILTERS; m++) {
        uint32_t left   = bins[m];
        uint32_t center = bins[m + 1];
        uint32_t right  = bins[m + 2];

        if (right <= left) right = left + 1;
        if (center < left)  center = left;
        if (center > right) center = right;

        s_mel_filter_pos[m]     = left;
        s_mel_filter_lengths[m] = right - left + 1;

        /* 上升斜坡: left → center */
        for (uint32_t k = left; k <= center; k++) {
            s_mel_coefs[idx++] = (center > left)
                ? (float32_t)(k - left) / (float32_t)(center - left)
                : 1.0f;
        }
        /* 下降斜坡: center+1 → right */
        for (uint32_t k = center + 1; k <= right; k++) {
            s_mel_coefs[idx++] = (right > center)
                ? (float32_t)(right - k) / (float32_t)(right - center)
                : 0.0f;
        }
    }

    return 0;
}

/* ======================== 公有接口 ======================== */

int audio_features_init(void)
{
    if (s_initialized) return 0;

    /* 分配工作缓冲 */
    s_hamming_window = pvPortMalloc(MFCC_FFT_LEN * sizeof(float32_t));
    s_dct_matrix     = pvPortMalloc(MFCC_NUM_COEFFS * MFCC_NUM_MEL_FILTERS * sizeof(float32_t));
    s_frame_buf      = pvPortMalloc(MFCC_FFT_LEN * sizeof(float32_t));
    s_tmp_buf        = pvPortMalloc((MFCC_FFT_LEN + 2) * sizeof(float32_t));

    if (!s_hamming_window || !s_dct_matrix || !s_frame_buf || !s_tmp_buf) {
        return -1;
    }

    /* 计算Hamming窗 */
    compute_hamming(s_hamming_window, MFCC_FFT_LEN);

    /* 计算DCT-II矩阵 */
    compute_dct_matrix(s_dct_matrix, MFCC_NUM_COEFFS, MFCC_NUM_MEL_FILTERS);

    /* 计算Mel三角滤波器组 */
    if (compute_mel_filters() != 0) {
        return -1;
    }

    /* 初始化CMSIS-DSP MFCC实例 */
    arm_status status = arm_mfcc_init_f32(
        &s_mfcc_inst,
        MFCC_FFT_LEN,
        MFCC_NUM_MEL_FILTERS,
        MFCC_NUM_COEFFS,
        s_dct_matrix,
        s_mel_filter_pos,
        s_mel_filter_lengths,
        s_mel_coefs,
        s_hamming_window
    );

    if (status != ARM_MATH_SUCCESS) {
        return -1;
    }

    s_initialized = 1;
    return 0;
}

void audio_features_compute_frame(const int16_t *mono_pcm, float32_t *mfcc_out)
{
    if (!s_initialized) return;

    /* int16 → float32 (Q15格式, 范围 [-1.0, 1.0)) */
    arm_q15_to_float((const q15_t *)mono_pcm, s_frame_buf, MFCC_FFT_LEN);

    /* arm_mfcc_f32 内部会归一化, Q15范围不影响最终MFCC值 */
    /* 流水线: 归一化 → Hamming窗 → RFFT → 幅度谱 → Mel滤波 → log → DCT */
    arm_mfcc_f32(&s_mfcc_inst, s_frame_buf, mfcc_out, s_tmp_buf);
}

uint8_t audio_features_is_ready(void)
{
    return s_initialized;
}
