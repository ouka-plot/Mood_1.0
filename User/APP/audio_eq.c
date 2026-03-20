/**
 ****************************************************************************************************
 * @file        audio_eq.c
 * @brief       多频段音频数字均衡器（EQ）模块实现
 *
 * @details
 * 1) 算法结构：
 *    - 采用 5 段串联（Cascade）的 Biquad IIR 滤波器。
 *    - 频段拓扑固定为：LowShelf + Peaking + Peaking + Peaking + HighShelf。
 *
 * 2) 配置模型：
 *    - 运行时参数保存在 s_runtime（开关、采样率、每段增益/频率/Q）。
 *    - 参数更新不立即重算系数，仅置 dirty 标志；在音频处理入口按需重算。
 *
 * 3) 并发模型：
 *    - 音频线程持续调用 audio_eq_process_buffer()。
 *    - UI/UART 等线程可修改 EQ 参数。
 *    - 使用 FreeRTOS 临界区保护共享配置拷贝与写入，避免竞态。
 *
 * 4) 数值模型：
 *    - 系数计算采用 Audio EQ Cookbook 公式。
 *    - 内部处理用 float，输出回写前钳位到 int16，防止溢出失真。
 *
 * 5) 支持格式：
 *    - 支持 16-bit 和 24-bit（按高 16 位处理）PCM。
 *    - 支持单声道/双声道。
 ****************************************************************************************************
 */

#include "./APP/audio_eq.h"

#include <math.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

/*
 * EQ 参数安全边界（对外输入的物理/工程约束）
 * - 增益单位：dB*10（例如 +35 表示 +3.5dB）
 * - Q 单位：Q*100（例如 120 表示 1.20）
 */
#define AUDIO_EQ_MIN_GAIN_DB_X10   (-120)    /* 最小增益：-12.0 dB */
#define AUDIO_EQ_MAX_GAIN_DB_X10   (120)     /* 最大增益：+12.0 dB */
#define AUDIO_EQ_MIN_FREQ_HZ       20U       /* 最小中心频率：20 Hz */
#define AUDIO_EQ_MAX_FREQ_HZ       20000U    /* 最大中心频率：20,000 Hz */
#define AUDIO_EQ_MIN_Q_X100        30U       /* 最小Q：0.30 */
#define AUDIO_EQ_MAX_Q_X100        400U      /* 最大Q：4.00 */

/*
 * 滤波器类型：
 * - 低频段：Low Shelf（低频搁架）
 * - 中频段：Peaking（峰值）
 * - 高频段：High Shelf（高频搁架）
 */
typedef enum
{
    AUDIO_EQ_FILTER_LOW_SHELF = 0,
    AUDIO_EQ_FILTER_PEAKING = 1,
    AUDIO_EQ_FILTER_HIGH_SHELF = 2
} audio_eq_filter_type_t;

/*
 * 标准二阶 IIR（Biquad）归一化系数：
 * y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
 * 注意：a0 已在计算阶段归一化为 1，因此结构体中不保存 a0。
 */
typedef struct
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
} audio_eq_biquad_coeffs_t;

/*
 * Biquad 延迟状态（每个频段、每个声道各一份）
 * x1/x2：历史输入；y1/y2：历史输出
 */
typedef struct
{
    float x1;   /* x[n-1] */
    float x2;   /* x[n-2] */
    float y1;   /* y[n-1] */
    float y2;   /* y[n-2] */
} audio_eq_biquad_state_t;

/*
 * EQ 运行时配置：
 * - dirty=1 表示“配置已变化，系数待刷新”
 */
typedef struct
{
    uint8_t enabled;                                /* 1=启用EQ，0=旁路 */
    uint32_t sample_rate;                           /* 当前音频采样率 */
    int16_t gain_db_x10[AUDIO_EQ_BAND_COUNT];       /* 各频段增益，单位 dB*10 */
    uint16_t freq_hz[AUDIO_EQ_BAND_COUNT];          /* 各频段中心频率，单位 Hz */
    uint16_t q_x100[AUDIO_EQ_BAND_COUNT];           /* 各频段Q，单位 Q*100 */
    uint8_t dirty;                                  /* 参数脏标志 */
} audio_eq_runtime_t;

/* 全局静态状态 */
static audio_eq_runtime_t s_runtime;
static audio_eq_biquad_coeffs_t s_coeffs[AUDIO_EQ_BAND_COUNT];//Biquad 滤波器系数（b0/b1/b2/a1/a2）
static audio_eq_biquad_state_t s_left_state[AUDIO_EQ_BAND_COUNT];//左声道的延迟历史状态（x1/x2/y1/y2）
static audio_eq_biquad_state_t s_right_state[AUDIO_EQ_BAND_COUNT];//右声道的延迟历史状态（x1/x2/y1/y2）

/* 5 段固定拓扑 */
static const audio_eq_filter_type_t s_filter_types[AUDIO_EQ_BAND_COUNT] = {
    AUDIO_EQ_FILTER_LOW_SHELF,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_HIGH_SHELF
};

/* ========================== 预设参数表 ========================== */

/* 增益预设：单位 dB*10 */
//根据不同场景预设增益
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

/* 默认频率与Q（Flat及兜底） */
static const uint16_t s_default_freq[AUDIO_EQ_BAND_COUNT] = { 60U, 250U, 1000U, 4000U, 12000U };
static const uint16_t s_default_q[AUDIO_EQ_BAND_COUNT]    = { 70U, 100U, 100U, 100U, 70U };

/* 各预设对应的频率/Q（可与默认不同） */
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

/* 预设索引表：preset_id -> (gain, freq, q) */
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
 * @brief 浮点钳位：将 value 限制在 [min_value, max_value]
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
 * @brief 16 位整型钳位：避免回写 PCM 时溢出
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
 * @brief 清空所有频段、所有声道的滤波历史状态
 * @note  常在参数突变、开关EQ时调用，降低突变引起的爆音概率
 */
static void audio_eq_reset_states(void)
{
    memset(s_left_state, 0, sizeof(s_left_state));
    memset(s_right_state, 0, sizeof(s_right_state));
}

/**
 * @brief 标记“配置已变化”，延迟到音频处理入口再统一刷新系数，防止在这个期间用户还在下发串口命令
 */
static void audio_eq_mark_dirty(void)
{
    s_runtime.dirty = 1U;
}

/**
 * @brief 装载默认参数（Flat）
 */
static void audio_eq_load_defaults(void)
{
    s_runtime.enabled = 1U;
    s_runtime.sample_rate = 44100U;
    //默认增益是0
    memcpy(s_runtime.gain_db_x10, s_flat_gain, sizeof(s_flat_gain));
    memcpy(s_runtime.freq_hz, s_default_freq, sizeof(s_default_freq));
    memcpy(s_runtime.q_x100, s_default_q, sizeof(s_default_q));
    audio_eq_mark_dirty();
}

/**
 * @brief 依据滤波器类型和参数计算 Biquad 归一化系数
 *
 * @param type        滤波器类型（LowShelf/Peaking/HighShelf）
 * @param sample_rate 采样率（Hz）
 * @param freq_hz     中心频率（Hz）
 * @param gain_db     增益（dB）
 * @param q_value     Q 值
 * @param coeff       输出系数
 *
 * @note
 * - 使用 Audio EQ Cookbook 公式。
 * - 内部先做参数钳位，避免极端值导致不稳定。
 * - 若 a0 接近 0，降级为单位增益直通，防止数值异常。
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
    const float a = powf(10.0f, gain_db / 40.0f); //x的y次方
    const float alpha = sin_w0 / (2.0f * clamped_q);
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;

    if (type == AUDIO_EQ_FILTER_PEAKING)//peak
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
        const float root_a = sqrtf(a);//算平方根
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
        /* 保护：异常参数导致分母接近0时，退化为直通 */
        coeff->b0 = 1.0f;
        coeff->b1 = 0.0f;
        coeff->b2 = 0.0f;
        coeff->a1 = 0.0f;
        coeff->a2 = 0.0f;
        return;
    }

    /* 归一化（a0 -> 1） */
    coeff->b0 = b0 / a0;
    coeff->b1 = b1 / a0;
    coeff->b2 = b2 / a0;
    coeff->a1 = a1 / a0;
    coeff->a2 = a2 / a0;
}

/**
 * @brief 若 dirty=1，则刷新全部频段系数
 *
 * @note
 * 1) 先在临界区内做 runtime 快照并清 dirty。
 * 2) 再在临界区外做较重的浮点计算，减少临界区占用时间。
 * 3) 重算后清历史状态，降低参数突变导致的瞬态冲击。
 */
static void audio_eq_refresh_if_needed(void)
{
    uint8_t band;
    audio_eq_runtime_t snapshot;

    if (s_runtime.dirty == 0U)
    {
        return;
    }
    //临界区
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

    audio_eq_reset_states();
}

/**
 * @brief 单个样本通过单个 Biquad 的计算
 *
 * @param input 输入样本
 * @param state 该频段对应声道的历史状态
 * @param coeff 该频段当前系数
 * @return      滤波输出
 */
static float audio_eq_apply_sample(float input, audio_eq_biquad_state_t *state, const audio_eq_biquad_coeffs_t *coeff)
{
    float output = (coeff->b0 * input) + (coeff->b1 * state->x1) + (coeff->b2 * state->x2)
                 - (coeff->a1 * state->y1) - (coeff->a2 * state->y2);

    /* 更新延迟线 */
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;
    return output;
}

/**
 * @brief 在“已进入临界区”的前提下应用预设参数
 */

static void audio_eq_apply_preset_locked(const audio_eq_preset_def_t *preset)
{
    memcpy(s_runtime.gain_db_x10, preset->gain, sizeof(s_runtime.gain_db_x10));
    memcpy(s_runtime.freq_hz, preset->freq, sizeof(s_runtime.freq_hz));
    memcpy(s_runtime.q_x100, preset->q, sizeof(s_runtime.q_x100));
    audio_eq_mark_dirty();
}

/* ========================== 对外接口实现 ========================== */

/**
 * @brief EQ 模块初始化
 * @note  初始化后默认启用 Flat 预设，采样率默认 44.1k
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
 * @brief 设置音频采样率
 * @note  建议在每首歌解析出采样率后调用
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
 * @brief 启用/禁用 EQ
 * @note  关闭时清状态，避免再次打开时带入旧延迟线
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
 * @brief 读取 EQ 开关状态
 */
uint8_t audio_eq_is_enabled(void)
{
    return s_runtime.enabled;
}

/**
 * @brief 设置某频段增益（dB*10）
 * @return 1=成功，0=band_index 越界
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
 * @brief 设置某频段中心频率（Hz）
 * @return 1=成功，0=band_index 越界
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
 * @brief 设置某频段Q（Q*100）
 * @return 1=成功，0=band_index 越界
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
 * @brief 应用预设
 * @param preset_id 预设索引（0 ~ AUDIO_EQ_PRESET_COUNT-1）
 * @return 1=成功，0=越界
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
 * @brief 获取当前 EQ 状态快照（供 UI/UART 查询）
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
 * @brief 对一整块 PCM 数据做就地 EQ 处理
 *
 * @param buf             音频缓冲区首地址
 * @param byte_count      缓冲字节数
 * @param bits_per_sample 位宽（16 或 24）
 * @param channels        声道数（1 或 2）
 *
 * @note
 * - 仅当 EQ 开启且格式受支持时处理，否则直接返回。
 * - 24-bit 路径按“每样本占 4 字节、取高16位”规则处理。
 * - 处理顺序：逐帧 -> 左右声道 -> 串联 5 个频段。
 */
void audio_eq_process_buffer(uint8_t *buf, uint32_t byte_count, uint8_t bits_per_sample, uint8_t channels)
{
    uint32_t frame_count;
    uint32_t frame_index;
    int16_t *samples;
    uint8_t band;

    /* 基础输入检查 */
    if ((buf == NULL) || (byte_count == 0U))
    {
        return;
    }

    /* EQ 关闭 或 位宽不支持：直接旁路 */
    if ((s_runtime.enabled == 0U) || ((bits_per_sample != 16U) && (bits_per_sample != 24U)))
    {
        return;
    }

    /* 仅支持单/双声道 */
    if ((channels != 1U) && (channels != 2U))
    {
        return;
    }

    /* 参数有变更时先刷新系数 */
    audio_eq_refresh_if_needed();

    samples = (int16_t *)buf;

    if (bits_per_sample == 24U)
    {
        /*
         * 24-bit/32-bit 封装路径：
         * - 每个样本占 4 字节（两个 int16 单元）
         * - 这里按约定提取“高 16 位”参与滤波
         * - 处理后再写回对应高 16 位位置
         */
        uint32_t stride = (uint32_t)channels * 2U; /* 每帧包含的 int16 单元数 */
        frame_count = byte_count / ((uint32_t)channels * 4U);

        for (frame_index = 0U; frame_index < frame_count; frame_index++)
        {
            uint32_t base = frame_index * stride;
            float left = (float)samples[base];
            float right = left;

            if (channels == 2U)
            {
                right = (float)samples[base + 2U];
            }

            /* 串联通过 5 个频段 */
            for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
            {
                left = audio_eq_apply_sample(left, &s_left_state[band], &s_coeffs[band]);

                if (channels == 2U)
                {
                    right = audio_eq_apply_sample(right, &s_right_state[band], &s_coeffs[band]);
                }
            }

            /* 回写并钳位 */
            samples[base] = audio_eq_clamp_i16((int32_t)lrintf(left));

            if (channels == 2U)
            {
                samples[base + 2U] = audio_eq_clamp_i16((int32_t)lrintf(right));
            }
        }
    }
    else
    {
        /* 16-bit 路径：样本连续存放 */
        frame_count = byte_count / (uint32_t)(channels * sizeof(int16_t));

        for (frame_index = 0U; frame_index < frame_count; frame_index++)
        {
            float left = (float)samples[frame_index * channels];
            float right = left;

            if (channels == 2U)
            {
                right = (float)samples[(frame_index * channels) + 1U];
            }

            /* 串联通过 5 个频段 */
            for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
            {
                left = audio_eq_apply_sample(left, &s_left_state[band], &s_coeffs[band]);

                if (channels == 2U)
                {
                    right = audio_eq_apply_sample(right, &s_right_state[band], &s_coeffs[band]);
                }
            }

            /* 回写并钳位 */
            samples[frame_index * channels] = audio_eq_clamp_i16((int32_t)lrintf(left));

            if (channels == 2U)
            {
                samples[(frame_index * channels) + 1U] = audio_eq_clamp_i16((int32_t)lrintf(right));
            }
        }
    }
}
