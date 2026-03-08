#ifndef __AUDIO_EQ_H
#define __AUDIO_EQ_H

#include "./SYSTEM/SYS/sys.h"

#define AUDIO_EQ_BAND_COUNT    5U

typedef struct
{
    uint8_t enabled;
    uint32_t sample_rate;
    int16_t gain_db_x10[AUDIO_EQ_BAND_COUNT];
    uint16_t freq_hz[AUDIO_EQ_BAND_COUNT];
    uint16_t q_x100[AUDIO_EQ_BAND_COUNT];
} audio_eq_status_t;

void audio_eq_init(void);
void audio_eq_set_sample_rate(uint32_t sample_rate);
void audio_eq_set_enabled(uint8_t enabled);
uint8_t audio_eq_is_enabled(void);
uint8_t audio_eq_set_band_gain(uint8_t band_index, int16_t gain_db_x10);
uint8_t audio_eq_set_band_freq(uint8_t band_index, uint16_t freq_hz);
uint8_t audio_eq_set_band_q(uint8_t band_index, uint16_t q_x100);
uint8_t audio_eq_set_preset(uint8_t preset_id);
void audio_eq_get_status(audio_eq_status_t *status);
void audio_eq_process_buffer(uint8_t *buf, uint32_t byte_count, uint8_t bits_per_sample, uint8_t channels);

#endif