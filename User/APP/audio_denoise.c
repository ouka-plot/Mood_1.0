/**
 ****************************************************************************************************
 * @file        audio_denoise.c
 * @brief       频域谱减法降噪实现
 * @details     流程 (每帧512样本):
 *              1. int16 → float, 加Hann窗
 *              2. RFFT → 复数频谱 [257 bins]
 *              3. 计算幅度谱 |X(k)|
 *              4. 前N帧: 累积噪声谱估计 noise_psd[k]
 *              5. 之后: 谱减法增益 G(k) = max(1 - α·noise/|X|, β)
 *              6. 乘增益到复数频谱
 *              7. IRFFT → float → int16 输出
 *
 *              内存: FFT缓冲 ~4KB + 噪声谱 ~1KB, 从CCM heap分配
 ****************************************************************************************************
 */

#include "audio_denoise.h"
#include "dsp/transform_functions.h"
#include "FreeRTOS.h"
#include <math.h>
#include <string.h>

/* ======================== 私有变量 ======================== */
static uint8_t s_initialized = 0;
static uint32_t s_frame_count = 0;    /* 已处理帧计数 */

/* CMSIS-DSP FFT 实例 */
static arm_rfft_fast_instance_f32 s_rfft_inst;

/* 工作缓冲 (CCM heap) */
static float32_t *s_fft_buf   = NULL;   /* [DENOISE_FFT_LEN] 时域/FFT输入 */
static float32_t *s_fft_out   = NULL;   /* [DENOISE_FFT_LEN] FFT输出(复数) */
static float32_t *s_mag       = NULL;   /* [DENOISE_FFT_LEN/2+1] 幅度谱 */
static float32_t *s_noise_psd = NULL;   /* [DENOISE_FFT_LEN/2+1] 噪声谱估计 */
static float32_t *s_window    = NULL;   /* [DENOISE_FFT_LEN] Hann窗 */

#define NUM_BINS    (DENOISE_FFT_LEN / 2 + 1)  /* 257 */

/* ======================== 私有函数 ======================== */

static void compute_hann_window(float32_t *win, uint32_t len)
{
    float32_t denom = (float32_t)(len - 1);
    for (uint32_t i = 0; i < len; i++) {
        win[i] = 0.5f * (1.0f - cosf(2.0f * PI * (float32_t)i / denom));
    }
}

/**
 * @brief  从RFFT输出提取幅度谱
 * @note   CMSIS RFFT输出格式: [Re(0), Re(N/2), Re(1), Im(1), Re(2), Im(2), ...]
 */
static void compute_magnitude(const float32_t *fft_out, float32_t *mag, uint32_t n_fft)
{
    uint32_t n_bins = n_fft / 2 + 1;

    /* DC bin (纯实数) */
    mag[0] = fabsf(fft_out[0]);

    /* Nyquist bin (纯实数, 存在 fft_out[1]) */
    mag[n_fft / 2] = fabsf(fft_out[1]);

    /* 其余复数 bin */
    for (uint32_t k = 1; k < n_fft / 2; k++) {
        float32_t re = fft_out[2 * k];
        float32_t im = fft_out[2 * k + 1];
        mag[k] = sqrtf(re * re + im * im);
    }
}

/**
 * @brief  对复数频谱乘以实数增益
 */
static void apply_gain(float32_t *fft_out, const float32_t *gain, uint32_t n_fft)
{
    /* DC */
    fft_out[0] *= gain[0];
    /* Nyquist */
    fft_out[1] *= gain[n_fft / 2];

    /* 复数 bins */
    for (uint32_t k = 1; k < n_fft / 2; k++) {
        fft_out[2 * k]     *= gain[k];
        fft_out[2 * k + 1] *= gain[k];
    }
}

/* ======================== 公有接口 ======================== */

int audio_denoise_init(void)
{
    if (s_initialized) return 0;

    /* 初始化RFFT */
    arm_status status = arm_rfft_fast_init_f32(&s_rfft_inst, DENOISE_FFT_LEN);
    if (status != ARM_MATH_SUCCESS) return -1;

    /* 分配缓冲 */
    s_fft_buf   = pvPortMalloc(DENOISE_FFT_LEN * sizeof(float32_t));
    s_fft_out   = pvPortMalloc(DENOISE_FFT_LEN * sizeof(float32_t));
    s_mag       = pvPortMalloc(NUM_BINS * sizeof(float32_t));
    s_noise_psd = pvPortMalloc(NUM_BINS * sizeof(float32_t));
    s_window    = pvPortMalloc(DENOISE_FFT_LEN * sizeof(float32_t));

    if (!s_fft_buf || !s_fft_out || !s_mag || !s_noise_psd || !s_window) {
        return -1;
    }

    memset(s_noise_psd, 0, NUM_BINS * sizeof(float32_t));
    compute_hann_window(s_window, DENOISE_FFT_LEN);

    s_frame_count = 0;
    s_initialized = 1;
    return 0;
}

void audio_denoise_process(int16_t *frame)
{
    if (!s_initialized) return;

    /* 1) int16 → float, 加窗 */
    for (uint32_t i = 0; i < DENOISE_FFT_LEN; i++) {
        s_fft_buf[i] = (float32_t)frame[i] * s_window[i];
    }

    /* 2) RFFT */
    arm_rfft_fast_f32(&s_rfft_inst, s_fft_buf, s_fft_out, 0);  /* 0=正变换 */

    /* 3) 幅度谱 */
    compute_magnitude(s_fft_out, s_mag, DENOISE_FFT_LEN);

    /* 4) 噪声估计阶段: 前N帧累积平均 */
    if (s_frame_count < DENOISE_NOISE_FRAMES) {
        float32_t alpha = 1.0f / (float32_t)(s_frame_count + 1);
        float32_t beta  = 1.0f - alpha;
        for (uint32_t k = 0; k < NUM_BINS; k++) {
            s_noise_psd[k] = beta * s_noise_psd[k] + alpha * s_mag[k];
        }
        s_frame_count++;
        /* 噪声估计期不做降噪, 输出原始帧 */
        return;
    }

    /* 5) 谱减法增益 (复用 s_mag 存储增益) */
    for (uint32_t k = 0; k < NUM_BINS; k++) {
        float32_t gain;
        if (s_mag[k] > 1e-10f) {
            gain = 1.0f - DENOISE_ALPHA * (s_noise_psd[k] / s_mag[k]);
        } else {
            gain = DENOISE_BETA;
        }
        /* 下限裁剪 */
        if (gain < DENOISE_BETA) gain = DENOISE_BETA;
        s_mag[k] = gain;  /* 复用 mag 数组存增益 */
    }

    /* 6) 对频谱乘增益 */
    apply_gain(s_fft_out, s_mag, DENOISE_FFT_LEN);

    /* 7) IRFFT → 时域 */
    arm_rfft_fast_f32(&s_rfft_inst, s_fft_out, s_fft_buf, 1);  /* 1=逆变换 */

    /* 8) float → int16 (带饱和) */
    for (uint32_t i = 0; i < DENOISE_FFT_LEN; i++) {
        float32_t val = s_fft_buf[i];
        if (val > 32767.0f)       val = 32767.0f;
        else if (val < -32768.0f) val = -32768.0f;
        frame[i] = (int16_t)val;
    }

    s_frame_count++;
}

void audio_denoise_reset(void)
{
    s_frame_count = 0;
    if (s_noise_psd) {
        memset(s_noise_psd, 0, NUM_BINS * sizeof(float32_t));
    }
}

uint8_t audio_denoise_is_ready(void)
{
    return s_initialized;
}
