#include "./APP/audio_eq.h"

#include <math.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#define AUDIO_EQ_MIN_GAIN_DB_X10   (-120)
#define AUDIO_EQ_MAX_GAIN_DB_X10   (120)
#define AUDIO_EQ_MIN_FREQ_HZ       20U
#define AUDIO_EQ_MAX_FREQ_HZ       20000U
#define AUDIO_EQ_MIN_Q_X100        30U
#define AUDIO_EQ_MAX_Q_X100        400U

typedef enum
{
    AUDIO_EQ_FILTER_LOW_SHELF = 0,
    AUDIO_EQ_FILTER_PEAKING = 1,
    AUDIO_EQ_FILTER_HIGH_SHELF = 2
} audio_eq_filter_type_t;

typedef struct
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
} audio_eq_biquad_coeffs_t;

typedef struct
{
    float x1;
    float x2;
    float y1;
    float y2;
} audio_eq_biquad_state_t;

typedef struct
{
    uint8_t enabled;
    uint32_t sample_rate;
    int16_t gain_db_x10[AUDIO_EQ_BAND_COUNT];
    uint16_t freq_hz[AUDIO_EQ_BAND_COUNT];
    uint16_t q_x100[AUDIO_EQ_BAND_COUNT];
    uint8_t dirty;
} audio_eq_runtime_t;

static audio_eq_runtime_t s_runtime;
static audio_eq_biquad_coeffs_t s_coeffs[AUDIO_EQ_BAND_COUNT];
static audio_eq_biquad_state_t s_left_state[AUDIO_EQ_BAND_COUNT];
static audio_eq_biquad_state_t s_right_state[AUDIO_EQ_BAND_COUNT];

static const audio_eq_filter_type_t s_filter_types[AUDIO_EQ_BAND_COUNT] = {
    AUDIO_EQ_FILTER_LOW_SHELF,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_PEAKING,
    AUDIO_EQ_FILTER_HIGH_SHELF
};

static const int16_t s_flat_gain[AUDIO_EQ_BAND_COUNT] = { 0, 0, 0, 0, 0 };
static const int16_t s_bass_gain[AUDIO_EQ_BAND_COUNT] = { 45, 20, 0, -10, -15 };
static const int16_t s_vocal_gain[AUDIO_EQ_BAND_COUNT] = { -20, -10, 25, 30, 10 };
static const int16_t s_treble_gain[AUDIO_EQ_BAND_COUNT] = { -10, 0, 5, 20, 40 };
static const int16_t s_vshape_gain[AUDIO_EQ_BAND_COUNT] = { 35, -15, -20, 10, 30 };

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

static void audio_eq_reset_states(void)
{
    memset(s_left_state, 0, sizeof(s_left_state));
    memset(s_right_state, 0, sizeof(s_right_state));
}

static void audio_eq_mark_dirty(void)
{
    s_runtime.dirty = 1U;
}

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

static void audio_eq_refresh_if_needed(void)
{
    uint8_t band;
    audio_eq_runtime_t snapshot;

    if (s_runtime.dirty == 0U)
    {
        return;
    }

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

static float audio_eq_apply_sample(float input, audio_eq_biquad_state_t *state, const audio_eq_biquad_coeffs_t *coeff)
{
    float output = (coeff->b0 * input) + (coeff->b1 * state->x1) + (coeff->b2 * state->x2)
                 - (coeff->a1 * state->y1) - (coeff->a2 * state->y2);

    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;
    return output;
}

static void audio_eq_apply_preset_locked(const int16_t gains[AUDIO_EQ_BAND_COUNT])
{
    memcpy(s_runtime.gain_db_x10, gains, sizeof(s_runtime.gain_db_x10));
    audio_eq_mark_dirty();
}

void audio_eq_init(void)
{
    memset(&s_runtime, 0, sizeof(s_runtime));
    memset(s_coeffs, 0, sizeof(s_coeffs));
    audio_eq_reset_states();
    audio_eq_load_defaults();
    audio_eq_refresh_if_needed();
}

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

uint8_t audio_eq_is_enabled(void)
{
    return s_runtime.enabled;
}

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

uint8_t audio_eq_set_preset(uint8_t preset_id)
{
    taskENTER_CRITICAL();

    switch (preset_id)
    {
    case 0U:
        audio_eq_apply_preset_locked(s_flat_gain);
        break;
    case 1U:
        audio_eq_apply_preset_locked(s_bass_gain);
        break;
    case 2U:
        audio_eq_apply_preset_locked(s_vocal_gain);
        break;
    case 3U:
        audio_eq_apply_preset_locked(s_treble_gain);
        break;
    case 4U:
        audio_eq_apply_preset_locked(s_vshape_gain);
        break;
    default:
        taskEXIT_CRITICAL();
        return 0U;
    }

    taskEXIT_CRITICAL();
    return 1U;
}

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

    if ((s_runtime.enabled == 0U) || (bits_per_sample != 16U))
    {
        return;
    }

    if ((channels != 1U) && (channels != 2U))
    {
        return;
    }

    audio_eq_refresh_if_needed();

    samples = (int16_t *)buf;
    frame_count = byte_count / (uint32_t)(channels * sizeof(int16_t));

    for (frame_index = 0U; frame_index < frame_count; frame_index++)
    {
        float left = (float)samples[frame_index * channels];
        float right = left;

        if (channels == 2U)
        {
            right = (float)samples[(frame_index * channels) + 1U];
        }

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
}