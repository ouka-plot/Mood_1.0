/**
 ****************************************************************************************************
 * @file        audio_eq.c
 * @brief       多频段音频数字均衡器 (EQ) 模块实现
 * @note        采用浮点双二阶(Biquad) IIR滤波器实现，提供预设音效(如Rock、Pop等)和自定义多频段(Band)调节功能。
 *              该模块在FreeRTOS任务上下文中运行(主要为Audio任务)，采用临界区保护配置更新机制。
 ****************************************************************************************************
 */

#include "./APP/audio_eq.h"

#include <math.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

/* 均衡器参数物理边界限制 */
#define AUDIO_EQ_MIN_GAIN_DB_X10   (-120)    /* 最小增益：-12.0 dB */
#define AUDIO_EQ_MAX_GAIN_DB_X10   (120)     /* 最大增益：+12.0 dB */
#define AUDIO_EQ_MIN_FREQ_HZ       20U       /* 最小中心频率：20 Hz */
#define AUDIO_EQ_MAX_FREQ_HZ       20000U    /* 最大中心频率：20,000 Hz */
#define AUDIO_EQ_MIN_Q_X100        30U       /* 最小频段带宽Q因子：0.30 */
#define AUDIO_EQ_MAX_Q_X100        400U      /* 最大频段带宽Q因子：4.00 */

/* 滤波器类型定义：低频架式、峰值/谷值、高频架式 */
typedef enum
{
    AUDIO_EQ_FILTER_LOW_SHELF = 0,      /* 低频展宽(低频搁架滤波器) */
    AUDIO_EQ_FILTER_PEAKING = 1,        /* 峰态(用于中间频段的带通/带阻滤波器) */
    AUDIO_EQ_FILTER_HIGH_SHELF = 2      /* 高频展宽(高频搁架滤波器) */
} audio_eq_filter_type_t;

/* IIR双二阶滤波器转换系数结构体 */
typedef struct
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
} audio_eq_biquad_coeffs_t;

/* IIR双二阶滤波器历史状态变量(延迟线) */
typedef struct
{
    float x1;   /* x[n-1]：上一个输入样本 */
    float x2;   /* x[n-2]：上上个输入样本 */
    float y1;   /* y[n-1]：上一个输出样本 */
    float y2;   /* y[n-2]：上上个输出样本 */
} audio_eq_biquad_state_t;

/* 音频均衡器运行时配置结构体 */
typedef struct
{
    uint8_t enabled;                                /* 是否开启均衡器特效 */
    uint32_t sample_rate;                           /* 当前音频采样率 */
    int16_t gain_db_x10[AUDIO_EQ_BAND_COUNT];       /* 各频段增益值（放大了10倍的dB值） */
    uint16_t freq_hz[AUDIO_EQ_BAND_COUNT];          /* 各频段中心频率 */
    uint16_t q_x100[AUDIO_EQ_BAND_COUNT];           /* 各频段Q系数（放大了100倍） */
    uint8_t dirty;                                  /* 脏数据标志置1表示外部配置发生变化，需重新计算系数 */
} audio_eq_runtime_t;

/* 全局静态变量分配区 */
static audio_eq_runtime_t s_runtime;                                    /* 存储现行EQ配置 */
static audio_eq_biquad_coeffs_t s_coeffs[AUDIO_EQ_BAND_COUNT];          /* 存放各频段现行系数 */
static audio_eq_biquad_state_t s_left_state[AUDIO_EQ_BAND_COUNT];       /* 左声道历史状态(用于连续滤波) */
static audio_eq_biquad_state_t s_right_state[AUDIO_EQ_BAND_COUNT];      /* 右声道历史状态 */

/* 分配给 5 个频段的固定滤波器拓扑： 低音段用LowShelf，高音段用HighShelf，中心段用Peaking */
static const audio_eq_filter_type_t s_filter_types[AUDIO_EQ_BAND_COUNT] = {
    AUDIO_EQ_FILTER_LOW_SHELF,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_HIGH_SHELF
};

/* ================== 下方为各种频段音效预设值的硬编码数据表 ================== */

static const int16_t s_flat_gain[AUDIO_EQ_BAND_COUNT]       = {   0,   0,   0,   0,   0 };
static const int16_t s_rock_gain[AUDIO_EQ_BAND_COUNT]       = {  80,  40, -20,  30,  60 };
static const int16_t s_pop_gain[AUDIO_EQ_BAND_COUNT]        = { -20,  15,  50,  40,  20 };
static const int16_t s_jazz_gain[AUDIO_EQ_BAND_COUNT]       = {  30,  25,  15, -15, -30 };
static const int16_t s_classic_gain[AUDIO_EQ_BAND_COUNT]    = {  25,  10, -25,  15,  30 };
static const int16_t s_electronic_gain[AUDIO_EQ_BAND_COUNT] = { 100,  50, -30,  40,  80 };
static const int16_t s_country_gain[AUDIO_EQ_BAND_COUNT]    = {  20,  35,  25,  15, -25 };
static const int16_t s_voice_gain[AUDIO_EQ_BAND_COUNT]      = { -50, -25,  70,  60, -30 };
static const int16_t s_subbass_gain[AUDIO_EQ_BAND_COUNT]    = { 120,  80,  15, -15,   0 };
static const int16_t s_hifi_gain[AUDIO_EQ_BAND_COUNT]       = {  40, -15,  10,  30,  20 };

typedef struct {
    const int16_t  *gain;
    const uint16_t *freq;
    const uint16_t *q;
} audio_eq_preset_def_t;

static const uint16_t s_default_freq[AUDIO_EQ_BAND_COUNT] = { 60U, 250U, 1000U, 4000U, 12000U };
static const uint16_t s_default_q[AUDIO_EQ_BAND_COUNT]    = { 70U, 100U, 100U, 100U, 70U };

/* Preset-specific freq/Q tables (NULL = use default) */
static const uint16_t s_rock_freq[AUDIO_EQ_BAND_COUNT]   = { 80U, 200U, 1000U, 3500U, 10000U };
static const uint16_t s_rock_q[AUDIO_EQ_BAND_COUNT]      = { 60U, 80U, 100U, 120U, 70U };
static const uint16_t s_pop_freq[AUDIO_EQ_BAND_COUNT]    = { 100U, 400U, 2000U, 5000U, 10000U };
static const uint16_t s_pop_q[AUDIO_EQ_BAND_COUNT]       = { 70U, 100U, 150U, 120U, 80U };
static const uint16_t s_jazz_freq[AUDIO_EQ_BAND_COUNT]   = { 80U, 300U, 1200U, 3000U, 8000U };
static const uint16_t s_jazz_q[AUDIO_EQ_BAND_COUNT]      = { 60U, 80U, 100U, 100U, 70U };
static const uint16_t s_classic_freq[AUDIO_EQ_BAND_COUNT] = { 60U, 250U, 1000U, 4000U, 12000U };
static const uint16_t s_classic_q[AUDIO_EQ_BAND_COUNT]   = { 50U, 80U, 80U, 100U, 50U };
static const uint16_t s_elec_freq[AUDIO_EQ_BAND_COUNT]   = { 40U, 150U, 800U, 5000U, 14000U };
static const uint16_t s_elec_q[AUDIO_EQ_BAND_COUNT]      = { 50U, 70U, 120U, 150U, 50U };
static const uint16_t s_country_freq[AUDIO_EQ_BAND_COUNT] = { 100U, 400U, 1500U, 3500U, 8000U };
static const uint16_t s_ctry_q[AUDIO_EQ_BAND_COUNT]      = { 60U, 100U, 100U, 120U, 70U };
static const uint16_t s_voice_freq[AUDIO_EQ_BAND_COUNT]  = { 100U, 300U, 2500U, 5000U, 10000U };
static const uint16_t s_voice_q[AUDIO_EQ_BAND_COUNT]     = { 70U, 120U, 200U, 150U, 80U };
static const uint16_t s_bass_freq[AUDIO_EQ_BAND_COUNT]   = { 40U, 100U, 500U, 3000U, 12000U };
static const uint16_t s_bass_q[AUDIO_EQ_BAND_COUNT]      = { 50U, 60U, 100U, 100U, 70U };
static const uint16_t s_hifi_freq[AUDIO_EQ_BAND_COUNT]   = { 60U, 200U, 1000U, 4000U, 10000U };
static const uint16_t s_hifi_q[AUDIO_EQ_BAND_COUNT]      = { 70U, 100U, 80U, 100U, 70U };

static const audio_eq_preset_def_t s_presets[] = {
    { s_flat_gain,       s_default_freq,  s_default_q },   /* 0: Flat */
    { s_rock_gain,       s_rock_freq,     s_rock_q },      /* 1: Rock */
    { s_pop_gain,        s_pop_freq,      s_pop_q },       /* 2: Pop */
    { s_jazz_gain,       s_jazz_freq,     s_jazz_q },      /* 3: Jazz */
    { s_classic_gain,    s_classic_freq,  s_classic_q },   /* 4: Classical */
    { s_electronic_gain, s_elec_freq,     s_elec_q },      /* 5: Electronic */
    { s_country_gain,    s_country_freq,  s_ctry_q },      /* 6: Country */
    { s_voice_gain,      s_voice_freq,    s_voice_q },     /* 7: Voice */
    { s_subbass_gain,    s_bass_freq,     s_bass_q },      /* 8: Bass Boost */
    { s_hifi_gain,       s_hifi_freq,     s_hifi_q },      /* 9: HiFi */
};

#define AUDIO_EQ_PRESET_COUNT  (sizeof(s_presets) / sizeof(s_presets[0]))

/**
 * @brief 将浮点数钳制(Clamp)在指定范围内
 */
static float audio_eq_clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

/**
 * @brief 将16位音频样本数值钳制在不溢出的安全范围 (-32768 ~ +32767)
 */
static int16_t audio_eq_clamp_i16(int32_t value)
{
    if (value > 32767)
    {
        return 32767;
    }

    if (value < -32768)
    {
        return -32768;
    }

    return (int16_t)value;
}

/**
 * @brief 清空双二阶滤波器的历史状态，通常在启/停EQ或切换参数时调用，防止发生瞬间爆音
 */
static void audio_eq_reset_states(void)
{
    memset(s_left_state, 0, sizeof(s_left_state));
    memset(s_right_state, 0, sizeof(s_right_state));
}

/**
 * @brief 标记配置被修改，提示音频处理环需在下一次采样计算前刷新滤波器系数系数
 */
static void audio_eq_mark_dirty(void)
{
    s_runtime.dirty = 1U;
}

/**
 * @brief 载入默认的 Flat (平坦) EQ 参数
 */
static void audio_eq_load_defaults(void)
{
    static const uint16_t default_freq[AUDIO_EQ_BAND_COUNT] = { 60U, 250U, 1000U, 4000U, 12000U };
    static const uint16_t default_q[AUDIO_EQ_BAND_COUNT] = { 70U, 100U, 100U, 100U, 70U };

    s_runtime.enabled = 1U;
    s_runtime.sample_rate = 44100U;
    memcpy(s_runtime.gain_db_x10, s_flat_gain, sizeof(s_flat_gain));
    memcpy(s_runtime.freq_hz, default_freq, sizeof(default_freq));
    memcpy(s_runtime.q_x100, default_q, sizeof(default_q));
    audio_eq_mark_dirty();
}

/**
 * @brief 核心数学函数: 根据 滤波器类型、采样率、中心频率、增益、Q值，重新映射出底层双二阶IIR滤波器系数(b0,b1,b2,a0,a1,a2)结构。
 *        使用了著名的 Audio EQ Cookbook 公式
 */
static void audio_eq_calc_coeff(audio_eq_filter_type_t type,
                                float sample_rate,
                                float freq_hz,
                                float gain_db,
                                float q_value,
                                audio_eq_biquad_coeffs_t *coeff)
{
    const float pi = 3.14159265358979323846f;
    const float clamped_sr = audio_eq_clampf(sample_rate, 8000.0f, 192000.0f);
    const float clamped_freq = audio_eq_clampf(freq_hz, 20.0f, (clamped_sr * 0.48f));
    const float clamped_q = audio_eq_clampf(q_value, 0.30f, 4.00f);
    const float w0 = 2.0f * pi * clamped_freq / clamped_sr;
    const float cos_w0 = cosf(w0);
    const float sin_w0 = sinf(w0);
    const float a = powf(10.0f, gain_db / 40.0f);
    const float alpha = sin_w0 / (2.0f * clamped_q);
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;

    if (type == AUDIO_EQ_FILTER_PEAKING)
    {
        b0 = 1.0f + alpha * a;
        b1 = -2.0f * cos_w0;
        b2 = 1.0f - alpha * a;
        a0 = 1.0f + alpha / a;
        a1 = -2.0f * cos_w0;
        a2 = 1.0f - alpha / a;
    }
    else
    {
        const float slope = clamped_q;
        const float root_a = sqrtf(a);
        const float shelf_term = sin_w0 * sqrtf(((a + (1.0f / a)) * ((1.0f / slope) - 1.0f)) + 2.0f);
        const float beta = 2.0f * root_a * (shelf_term * 0.5f);

        if (type == AUDIO_EQ_FILTER_LOW_SHELF)
        {
            b0 = a * ((a + 1.0f) - ((a - 1.0f) * cos_w0) + beta);
            b1 = 2.0f * a * ((a - 1.0f) - ((a + 1.0f) * cos_w0));
            b2 = a * ((a + 1.0f) - ((a - 1.0f) * cos_w0) - beta);
            a0 = (a + 1.0f) + ((a - 1.0f) * cos_w0) + beta;
            a1 = -2.0f * ((a - 1.0f) + ((a + 1.0f) * cos_w0));
            a2 = (a + 1.0f) + ((a - 1.0f) * cos_w0) - beta;
        }
        else
        {
            b0 = a * ((a + 1.0f) + ((a - 1.0f) * cos_w0) + beta);
            b1 = -2.0f * a * ((a - 1.0f) + ((a + 1.0f) * cos_w0));
            b2 = a * ((a + 1.0f) + ((a - 1.0f) * cos_w0) - beta);
            a0 = (a + 1.0f) - ((a - 1.0f) * cos_w0) + beta;
            a1 = 2.0f * ((a - 1.0f) - ((a + 1.0f) * cos_w0));
            a2 = (a + 1.0f) - ((a - 1.0f) * cos_w0) - beta;
        }
    }

    if (fabsf(a0) < 1e-9f)
    {
        coeff->b0 = 1.0f;
        coeff->b1 = 0.0f;
        coeff->b2 = 0.0f;
        coeff->a1 = 0.0f;
        coeff->a2 = 0.0f;
        return;
    }

    coeff->b0 = b0 / a0;
    coeff->b1 = b1 / a0;
    coeff->b2 = b2 / a0;
    coeff->a1 = a1 / a0;
    coeff->a2 = a2 / a0;
}

/**
 * @brief 在音频任务处理数据前进行拦截检查，如果参数改变了，就触发系数重新计算。
 * @note  由于音频中断(或轮询任务)运行极快，必须用临界区保护 snapshot 内存拷贝过程。
 */
static void audio_eq_refresh_if_needed(void)
{
    uint8_t band;
    audio_eq_runtime_t snapshot;

    if (s_runtime.dirty == 0U)
    {
        return;
    }

    /* 进出临界区，避免UI/串口等任务在执行一半时打断数据拷贝致参量失真 */
    taskENTER_CRITICAL();
    memcpy(&snapshot, &s_runtime, sizeof(snapshot));
    s_runtime.dirty = 0U;
    taskEXIT_CRITICAL();

    for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
    {
        audio_eq_calc_coeff(s_filter_types[band],
                            (float)snapshot.sample_rate,
                            (float)snapshot.freq_hz[band],
                            ((float)snapshot.gain_db_x10[band]) / 10.0f,
                            ((float)snapshot.q_x100[band]) / 100.0f,
                            &s_coeffs[band]);
    }

    /* 参数重算后，重置延迟历史点。防止因突变的系数带来巨大直流分量抖动(大爆音) */
    audio_eq_reset_states();
}

/**
 * @brief 使用差分方程式，对单个音频样本(Sample)做一次双二阶滤波过滤
 */
static float audio_eq_apply_sample(float input, audio_eq_biquad_state_t *state, const audio_eq_biquad_coeffs_t *coeff)
{
    float output = (coeff->b0 * input) + (coeff->b1 * state->x1) + (coeff->b2 * state->x2)
                 - (coeff->a1 * state->y1) - (coeff->a2 * state->y2);

    /* 更新差分延迟链 */
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;
    return output;
}

/**
 * @brief 加载预设音效
 */
static void audio_eq_apply_preset_locked(const audio_eq_preset_def_t *preset)
{
    memcpy(s_runtime.gain_db_x10, preset->gain, sizeof(s_runtime.gain_db_x10));
    memcpy(s_runtime.freq_hz, preset->freq, sizeof(s_runtime.freq_hz));
    memcpy(s_runtime.q_x100, preset->q, sizeof(s_runtime.q_x100));
    audio_eq_mark_dirty();
}

/* ================== 提供给外界调用的公共接口定义 ================== */

/**
 * @brief EQ 模块全局初始化
 */
void audio_eq_init(void)
{
    memset(&s_runtime, 0, sizeof(s_runtime));
    memset(s_coeffs, 0, sizeof(s_coeffs));
    audio_eq_reset_states();
    audio_eq_load_defaults();
    audio_eq_refresh_if_needed();
}

/**
 * @brief 设置当前音频采样率(受播放的WAV文件影响，播放前调用)
 */
void audio_eq_set_sample_rate(uint32_t sample_rate)
{
    if (sample_rate < 8000U)
    {
        sample_rate = 8000U;
    }

    if (sample_rate > 192000U)
    {
        sample_rate = 192000U;
    }

    taskENTER_CRITICAL();
    s_runtime.sample_rate = sample_rate;
    audio_eq_mark_dirty();
    taskEXIT_CRITICAL();
}

/**
 * @brief 开启或关闭软件 EQ 功能
 */
void audio_eq_set_enabled(uint8_t enabled)
{
    taskENTER_CRITICAL();
    s_runtime.enabled = enabled ? 1U : 0U;
    if (s_runtime.enabled == 0U)
    {
        audio_eq_reset_states();
    }
    taskEXIT_CRITICAL();
}

/**
 * @brief 查询软件 EQ 当前是否开启
 */
uint8_t audio_eq_is_enabled(void)
{
    return s_runtime.enabled;
}

/**
 * @brief 调节指定频段的增益强度 (Gain Db)
 */
uint8_t audio_eq_set_band_gain(uint8_t band_index, int16_t gain_db_x10)
{
    if (band_index >= AUDIO_EQ_BAND_COUNT)
    {
        return 0U;
    }

    if (gain_db_x10 < AUDIO_EQ_MIN_GAIN_DB_X10)
    {
        gain_db_x10 = AUDIO_EQ_MIN_GAIN_DB_X10;
    }
    else if (gain_db_x10 > AUDIO_EQ_MAX_GAIN_DB_X10)
    {
        gain_db_x10 = AUDIO_EQ_MAX_GAIN_DB_X10;
    }

    taskENTER_CRITICAL();
    s_runtime.gain_db_x10[band_index] = gain_db_x10;
    audio_eq_mark_dirty();
    taskEXIT_CRITICAL();
    return 1U;
}

/**
 * @brief 调节指定频段的中心频率 (Frequency Hz)
 */
uint8_t audio_eq_set_band_freq(uint8_t band_index, uint16_t freq_hz)
{
    if (band_index >= AUDIO_EQ_BAND_COUNT)
    {
        return 0U;
    }

    if (freq_hz < AUDIO_EQ_MIN_FREQ_HZ)
    {
        freq_hz = AUDIO_EQ_MIN_FREQ_HZ;
    }
    else if (freq_hz > AUDIO_EQ_MAX_FREQ_HZ)
    {
        freq_hz = AUDIO_EQ_MAX_FREQ_HZ;
    }

    taskENTER_CRITICAL();
    s_runtime.freq_hz[band_index] = freq_hz;
    audio_eq_mark_dirty();
    taskEXIT_CRITICAL();
    return 1U;
}

/**
 * @brief 调节指定频段的品质因数 (Q) / 带宽窄度
 */
uint8_t audio_eq_set_band_q(uint8_t band_index, uint16_t q_x100)
{
    if (band_index >= AUDIO_EQ_BAND_COUNT)
    {
        return 0U;
    }

    if (q_x100 < AUDIO_EQ_MIN_Q_X100)
    {
        q_x100 = AUDIO_EQ_MIN_Q_X100;
    }
    else if (q_x100 > AUDIO_EQ_MAX_Q_X100)
    {
        q_x100 = AUDIO_EQ_MAX_Q_X100;
    }

    taskENTER_CRITICAL();
    s_runtime.q_x100[band_index] = q_x100;
    audio_eq_mark_dirty();
    taskEXIT_CRITICAL();
    return 1U;
}

/**
 * @brief 一键应用预设方案
 */
uint8_t audio_eq_set_preset(uint8_t preset_id)
{
    if (preset_id >= AUDIO_EQ_PRESET_COUNT)
    {
        return 0U;
    }

    taskENTER_CRITICAL();
    audio_eq_apply_preset_locked(&s_presets[preset_id]);
    taskEXIT_CRITICAL();
    return 1U;
}

/**
 * @brief 获取当前EQ各频段参数和开关状态 (供UI更新显示用)
 */
void audio_eq_get_status(audio_eq_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    taskENTER_CRITICAL();
    status->enabled = s_runtime.enabled;
    status->sample_rate = s_runtime.sample_rate;
    memcpy(status->gain_db_x10, s_runtime.gain_db_x10, sizeof(status->gain_db_x10));
    memcpy(status->freq_hz, s_runtime.freq_hz, sizeof(status->freq_hz));
    memcpy(status->q_x100, s_runtime.q_x100, sizeof(status->q_x100));
    taskEXIT_CRITICAL();
}

/**
 * @brief 核心音频流处理函数。拦截来自WAV解码器的一整块PCM数据，就地(In-Place)进行EQ全频段滤波计算。
 * @note  由于该函数放在DMA发送准备的空隙中执行(CPU T_read < DMA T_play)，所以运算耗时极其关键。
 */
void audio_eq_process_buffer(uint8_t *buf, uint32_t byte_count, uint8_t bits_per_sample, uint8_t channels)
{
    uint32_t frame_count;
    uint32_t frame_index;
    int16_t *samples;
    uint8_t band;

    if ((buf == NULL) || (byte_count == 0U))
    {
        return;
    }

    if ((s_runtime.enabled == 0U) || ((bits_per_sample != 16U) && (bits_per_sample != 24U)))
    {
        return;
    }

    if ((channels != 1U) && (channels != 2U))
    {
        return;
    }

    /* 检查一下配置有没有被串口或UI修改过，如果有，重算滤波系数 */
    audio_eq_refresh_if_needed();

    samples = (int16_t *)buf;

    if (bits_per_sample == 24U)
    {
        /* 24-bit/32-bit模式处理: 在此模式下，每个样本在内存中占用4个字节(2个半字)。
         * STM32采用大端或小端对齐方式(取决于I2S配置)，有效的高16位通常位于第一个半字。
         * 我们仅提取那16位数据进行滤波并在处理后插回原位。 */
        uint32_t stride = (uint32_t)channels * 2U; /* int16_t units per frame */
        frame_count = byte_count / ((uint32_t)channels * 4U);

        for (frame_index = 0U; frame_index < frame_count; frame_index++)
        {
            uint32_t base = frame_index * stride;
            float left = (float)samples[base];
            float right = left;     /* 假设单声道时，右声道暂为左声道数据 */

            /* 如果是双声道，提取紧随其后的右声道高16位数据 */
            if (channels == 2U)
            {
                right = (float)samples[base + 2U];
            }

            /* 串联穿过 5 个 EQ 频段滤波器 (Cascade Filter) */
            for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
            {
                left = audio_eq_apply_sample(left, &s_left_state[band], &s_coeffs[band]);

                if (channels == 2U)
                {
                    right = audio_eq_apply_sample(right, &s_right_state[band], &s_coeffs[band]);
                }
            }

            /* 将浮点数钳回16位整形并写回原内存 */
            samples[base] = audio_eq_clamp_i16((int32_t)lrintf(left));

            if (channels == 2U)
            {
                samples[base + 2U] = audio_eq_clamp_i16((int32_t)lrintf(right));
            }
        }
    }
    else
    {
        /* 16-bit 模式：直接连续处理 */
        frame_count = byte_count / (uint32_t)(channels * sizeof(int16_t));

    for (frame_index = 0U; frame_index < frame_count; frame_index++)
    {
        float left = (float)samples[frame_index * channels];
        float right = left;

        if (channels == 2U)
        {
            right = (float)samples[(frame_index * channels) + 1U];
        }

        /* 串联穿过 5 个 EQ 频段滤波器 */
        for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
        {
            left = audio_eq_apply_sample(left, &s_left_state[band], &s_coeffs[band]);

            if (channels == 2U)
            {
                right = audio_eq_apply_sample(right, &s_right_state[band], &s_coeffs[band]);
            }
        }

        samples[frame_index * channels] = audio_eq_clamp_i16((int32_t)lrintf(left));

        if (channels == 2U)
        {
            samples[(frame_index * channels) + 1U] = audio_eq_clamp_i16((int32_t)lrintf(right));
        }
    }
    } /* end else (16-bit) */
}