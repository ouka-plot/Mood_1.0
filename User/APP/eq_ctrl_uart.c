#include "./APP/eq_ctrl_uart.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "./APP/audio_eq.h"
#include "./APP/audio_ui_bridge.h"
#include "./BSP/ES8388/es8388.h"

#define EQ_UART_INSTANCE                 USART2
#define EQ_UART_IRQn                     USART2_IRQn
#define EQ_UART_GPIO_PORT                GPIOA
#define EQ_UART_TX_PIN                   GPIO_PIN_2
#define EQ_UART_RX_PIN                   GPIO_PIN_3
#define EQ_UART_GPIO_AF                  GPIO_AF7_USART2
#define EQ_UART_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)
#define EQ_UART_CLK_ENABLE()             do { __HAL_RCC_USART2_CLK_ENABLE(); } while (0)

#define EQ_CTRL_LINE_MAX_LEN             96U

static UART_HandleTypeDef s_eq_uart_handle;
static volatile uint8_t s_rx_line_ready;
static char s_rx_line[EQ_CTRL_LINE_MAX_LEN];
static char s_ready_line[EQ_CTRL_LINE_MAX_LEN];
static volatile uint16_t s_rx_line_len;

static void eq_ctrl_uart_send_status(void)
{
    audio_eq_status_t status;
    char buffer[96];
    uint8_t band;

    audio_eq_get_status(&status);

    /* First line: EQ ON/OFF */
    snprintf(buffer, sizeof(buffer), "EQ: %s\r\n",
             status.enabled ? "ON" : "OFF");
    eq_ctrl_uart_send_text(buffer);

    /* Per-band lines: BAND n: freq=xxx gain=xxx Q=xxx */
    for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
    {
        snprintf(buffer, sizeof(buffer),
                 "BAND %u: freq=%u gain=%d Q=%u\r\n",
                 (unsigned int)(band + 1U),
                 (unsigned int)status.freq_hz[band],
                 (int)status.gain_db_x10[band],
                 (unsigned int)status.q_x100[band]);
        eq_ctrl_uart_send_text(buffer);
    }

    /* Volume line */
    snprintf(buffer, sizeof(buffer), "VOL: %u\r\n",
             (unsigned int)g_audio_ui.vol_level);
    eq_ctrl_uart_send_text(buffer);
}

void eq_ctrl_uart_send_player_status(void)
{
    char buffer[96];

    snprintf(buffer, sizeof(buffer), "SONG: %s\r\n", g_audio_ui.song_name);
    eq_ctrl_uart_send_text(buffer);

    snprintf(buffer, sizeof(buffer), "TRACK: %u/%u\r\n",
             (unsigned int)g_audio_ui.cur_index,
             (unsigned int)g_audio_ui.total_files);
    eq_ctrl_uart_send_text(buffer);

    snprintf(buffer, sizeof(buffer), "TIME: %lu %lu\r\n",
             (unsigned long)g_audio_ui.cur_time,
             (unsigned long)g_audio_ui.total_time);
    eq_ctrl_uart_send_text(buffer);

    snprintf(buffer, sizeof(buffer), "PLAYING: %u\r\n",
             (unsigned int)g_audio_ui.is_playing);
    eq_ctrl_uart_send_text(buffer);

    snprintf(buffer, sizeof(buffer), "VOL: %u\r\n",
             (unsigned int)g_audio_ui.vol_level);
    eq_ctrl_uart_send_text(buffer);
}

static void eq_ctrl_uart_send_ok(void)
{
    eq_ctrl_uart_send_text("OK\r\n");
}

static void eq_ctrl_uart_send_ok_text(const char *text)
{
    char buffer[96];

    snprintf(buffer, sizeof(buffer), "OK %s\r\n", text);
    eq_ctrl_uart_send_text(buffer);
}

static void eq_ctrl_uart_send_error(const char *reason)
{
    char buffer[96];

    snprintf(buffer, sizeof(buffer), "ERR %s\r\n", reason);
    eq_ctrl_uart_send_text(buffer);
}

static void eq_ctrl_uart_format_gain_db(char *buffer, size_t buffer_size, int16_t gain_db_x10)
{
    snprintf(buffer,
             buffer_size,
             "%c%d.%ddB",
             gain_db_x10 >= 0 ? '+' : '-',
             abs((int)gain_db_x10 / 10),
             abs((int)gain_db_x10 % 10));
}

static void eq_ctrl_uart_format_q(char *buffer, size_t buffer_size, uint16_t q_x100)
{
    snprintf(buffer,
             buffer_size,
             "%u.%02u",
             (unsigned int)(q_x100 / 100U),
             (unsigned int)(q_x100 % 100U));
}

static void eq_ctrl_uart_to_upper(char *text)
{
    while ((*text) != '\0')
    {
        *text = (char)toupper((unsigned char)(*text));
        text++;
    }
}

static long eq_ctrl_uart_parse_long(const char *text, uint8_t *ok)
{
    char *end_ptr;
    long value;

    if ((text == NULL) || (*text == '\0'))
    {
        *ok = 0U;
        return 0;
    }

    value = strtol(text, &end_ptr, 10);
    if ((*end_ptr) != '\0')
    {
        *ok = 0U;
        return 0;
    }

    *ok = 1U;
    return value;
}

static void eq_ctrl_uart_handle_eq_cmd(char *args)
{
    char *subcmd;
    char *arg1;
    char *arg2;
    char *arg3;
    uint8_t ok;
    long value1;
    long value2;
    long value3;
    audio_eq_status_t status;
    char msg[96];
    char gain_text[16];
    char q_text[16];

    if (args == NULL)
    {
        eq_ctrl_uart_send_error("BAD_EQ");
        return;
    }

    subcmd = strtok(args, " ");
    if (subcmd == NULL)
    {
        eq_ctrl_uart_send_error("BAD_EQ");
        return;
    }

    eq_ctrl_uart_to_upper(subcmd);

    if (strcmp(subcmd, "ON") == 0)
    {
        audio_eq_set_enabled(1U);
        audio_ui_notify_eq_changed();
        eq_ctrl_uart_send_ok_text("EQ ON");
        return;
    }

    if (strcmp(subcmd, "OFF") == 0)
    {
        audio_eq_set_enabled(0U);
        audio_ui_notify_eq_changed();
        eq_ctrl_uart_send_ok_text("EQ OFF");
        return;
    }

    if (strcmp(subcmd, "STATUS") == 0)
    {
        eq_ctrl_uart_send_status();
        return;
    }

    if (strcmp(subcmd, "PRESET") == 0)
    {
        arg1 = strtok(NULL, " ");
        value1 = eq_ctrl_uart_parse_long(arg1, &ok);
        if ((ok == 0U) || (audio_eq_set_preset((uint8_t)value1) == 0U))
        {
            eq_ctrl_uart_send_error("BAD_PRESET");
            return;
        }

        audio_ui_notify_eq_changed();
        snprintf(msg, sizeof(msg), "PRESET %ld", value1);
        eq_ctrl_uart_send_ok_text(msg);
        return;
    }

    if (strcmp(subcmd, "ALL") == 0)
    {
        uint8_t band;

        for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
        {
            arg1 = strtok(NULL, " ");
            value1 = eq_ctrl_uart_parse_long(arg1, &ok);
            if (ok == 0U)
            {
                eq_ctrl_uart_send_error("BAD_GAIN");
                return;
            }

            audio_eq_set_band_gain(band, (int16_t)value1);
        }

        audio_ui_notify_eq_changed();
        audio_eq_get_status(&status);
        eq_ctrl_uart_format_gain_db(gain_text, sizeof(gain_text), status.gain_db_x10[0]);
        {
            char gain_text_2[16];
            char gain_text_3[16];
            char gain_text_4[16];
            char gain_text_5[16];

            eq_ctrl_uart_format_gain_db(gain_text_2, sizeof(gain_text_2), status.gain_db_x10[1]);
            eq_ctrl_uart_format_gain_db(gain_text_3, sizeof(gain_text_3), status.gain_db_x10[2]);
            eq_ctrl_uart_format_gain_db(gain_text_4, sizeof(gain_text_4), status.gain_db_x10[3]);
            eq_ctrl_uart_format_gain_db(gain_text_5, sizeof(gain_text_5), status.gain_db_x10[4]);
        snprintf(msg,
                 sizeof(msg),
                 "ALL B1=%s B2=%s B3=%s B4=%s B5=%s",
                 gain_text,
                 gain_text_2,
                 gain_text_3,
                 gain_text_4,
                 gain_text_5);
        }
        eq_ctrl_uart_send_ok_text(msg);
        return;
    }

    if ((strcmp(subcmd, "BAND") == 0) || (strcmp(subcmd, "FREQ") == 0) || (strcmp(subcmd, "Q") == 0))
    {
        arg1 = strtok(NULL, " ");
        arg2 = strtok(NULL, " ");
        value1 = eq_ctrl_uart_parse_long(arg1, &ok);
        if ((ok == 0U) || (value1 < 1) || (value1 > AUDIO_EQ_BAND_COUNT))
        {
            eq_ctrl_uart_send_error("BAD_BAND");
            return;
        }

        value2 = eq_ctrl_uart_parse_long(arg2, &ok);
        if (ok == 0U)
        {
            eq_ctrl_uart_send_error("BAD_VALUE");
            return;
        }

        if (strcmp(subcmd, "BAND") == 0)
        {
            audio_eq_set_band_gain((uint8_t)(value1 - 1), (int16_t)value2);
            audio_ui_notify_eq_changed();
            audio_eq_get_status(&status);
            eq_ctrl_uart_format_gain_db(gain_text, sizeof(gain_text), status.gain_db_x10[value1 - 1]);
            snprintf(msg, sizeof(msg), "B%ld GAIN %s", value1, gain_text);
            eq_ctrl_uart_send_ok_text(msg);
            return;
        }

        if (strcmp(subcmd, "FREQ") == 0)
        {
            audio_eq_set_band_freq((uint8_t)(value1 - 1), (uint16_t)value2);
            audio_ui_notify_eq_changed();
            audio_eq_get_status(&status);
            snprintf(msg,
                     sizeof(msg),
                     "B%ld FREQ %uHz",
                     value1,
                     (unsigned int)status.freq_hz[value1 - 1]);
            eq_ctrl_uart_send_ok_text(msg);
            return;
        }

        audio_eq_set_band_q((uint8_t)(value1 - 1), (uint16_t)value2);
        audio_ui_notify_eq_changed();
        audio_eq_get_status(&status);
        eq_ctrl_uart_format_q(q_text, sizeof(q_text), status.q_x100[value1 - 1]);
        snprintf(msg, sizeof(msg), "B%ld Q %s", value1, q_text);
        eq_ctrl_uart_send_ok_text(msg);
        return;
    }

    if (strcmp(subcmd, "CFG") == 0)
    {
        arg1 = strtok(NULL, " ");
        arg2 = strtok(NULL, " ");
        arg3 = strtok(NULL, " ");
        value1 = eq_ctrl_uart_parse_long(arg1, &ok);
        if ((ok == 0U) || (value1 < 1) || (value1 > AUDIO_EQ_BAND_COUNT))
        {
            eq_ctrl_uart_send_error("BAD_BAND");
            return;
        }

        value2 = eq_ctrl_uart_parse_long(arg2, &ok);
        if (ok == 0U)
        {
            eq_ctrl_uart_send_error("BAD_FREQ");
            return;
        }

        value3 = eq_ctrl_uart_parse_long(arg3, &ok);
        if (ok == 0U)
        {
            eq_ctrl_uart_send_error("BAD_Q");
            return;
        }

        audio_eq_set_band_freq((uint8_t)(value1 - 1), (uint16_t)value2);
        audio_eq_set_band_q((uint8_t)(value1 - 1), (uint16_t)value3);
        audio_ui_notify_eq_changed();
        audio_eq_get_status(&status);
        eq_ctrl_uart_format_q(q_text, sizeof(q_text), status.q_x100[value1 - 1]);
        snprintf(msg,
                 sizeof(msg),
                 "B%ld FREQ %uHz Q %s",
                 value1,
                 (unsigned int)status.freq_hz[value1 - 1],
                 q_text);
        eq_ctrl_uart_send_ok_text(msg);
        return;
    }

    {
        char errbuf[64];
        snprintf(errbuf, sizeof(errbuf), "UNKNOWN_EQ[%s]", subcmd ? subcmd : "NULL");
        eq_ctrl_uart_send_error(errbuf);
    }
}

static void eq_ctrl_uart_handle_line(char *line)
{
    char local_line[EQ_CTRL_LINE_MAX_LEN];
    char *cmd;
    char *args;
    uint8_t ok;
    long value;

    strncpy(local_line, line, sizeof(local_line) - 1U);
    local_line[sizeof(local_line) - 1U] = '\0';

    cmd = strtok(local_line, " ");
    if (cmd == NULL)
    {
        return;
    }

    eq_ctrl_uart_to_upper(cmd);
    args = strtok(NULL, "");

    if (strcmp(cmd, "EQ") == 0)
    {
        eq_ctrl_uart_handle_eq_cmd(args);
        return;
    }

    if (strcmp(cmd, "PING") == 0)
    {
        eq_ctrl_uart_send_text("PONG\r\n");
        return;
    }

    if (strcmp(cmd, "ID") == 0)
    {
        eq_ctrl_uart_send_text("STM32F407-EQ-BRIDGE\r\n");
        return;
    }

    if (strcmp(cmd, "VOL") == 0)
    {
        value = eq_ctrl_uart_parse_long(args, &ok);
        if ((ok == 0U) || (value < 0) || (value > 100))
        {
            eq_ctrl_uart_send_error("BAD_VOL");
            return;
        }

        /* 耳机和喇叭分开映射，喇叭功放余量小需要更低上限 */
        {
            uint8_t hp_vol  = (uint8_t)((value * 30U + 50U) / 100U);  /* 耳机 0-30 */
            uint8_t spk_vol = (uint8_t)((value * 19U + 50U) / 100U);  /* 喇叭 0-19 */
            if (value == 0)
            {
                /* 真正静音：DAC数字音量衰减到最大(-96dB) + 模拟音量归零 */
                es8388_write_reg(0x1A, 0xC0);
                es8388_write_reg(0x1B, 0xC0);
                es8388_hpvol_set(0);
                es8388_spkvol_set(0);
            }
            else
            {
                /* 恢复DAC数字音量(0dB)，设置模拟音量 */
                es8388_write_reg(0x1A, 0x00);
                es8388_write_reg(0x1B, 0x00);
                es8388_hpvol_set(hp_vol);
                es8388_spkvol_set(spk_vol);
            }
        }

        /* 通知UI显示音量弹出条 */
        audio_ui_set_volume((uint8_t)value);

        {
            char buf[32];
            snprintf(buf, sizeof(buf), "OK VOL %ld\r\n", value);
            eq_ctrl_uart_send_text(buf);
        }
        return;
    }

    if (strcmp(cmd, "PLAY") == 0)
    {
        g_audio_ui.ui_cmd = UI_CMD_PLAY;
        eq_ctrl_uart_send_ok_text("PLAY");
        return;
    }

    if (strcmp(cmd, "PAUSE") == 0)
    {
        g_audio_ui.ui_cmd = UI_CMD_PAUSE;
        eq_ctrl_uart_send_ok_text("PAUSE");
        return;
    }

    if (strcmp(cmd, "NEXT") == 0)
    {
        g_audio_ui.ui_cmd = UI_CMD_NEXT;
        eq_ctrl_uart_send_ok_text("NEXT");
        return;
    }

    if (strcmp(cmd, "PREV") == 0)
    {
        g_audio_ui.ui_cmd = UI_CMD_PREV;
        eq_ctrl_uart_send_ok_text("PREV");
        return;
    }

    if (strcmp(cmd, "STATUS") == 0)
    {
        eq_ctrl_uart_send_player_status();
        return;
    }

    if (strcmp(cmd, "HELP") == 0)
    {
        eq_ctrl_uart_send_text("PING|ID|STATUS|HELP\r\nVOL 0..100\r\nPLAY|PAUSE|NEXT|PREV\r\nEQ ON|OFF|STATUS|PRESET n\r\nEQ BAND n gain_x10|FREQ n hz|Q n q_x100|CFG n hz q_x100\r\n");
        return;
    }

    {
        char errbuf[64];
        snprintf(errbuf, sizeof(errbuf), "UNKNOWN_CMD[%s]", cmd ? cmd : "NULL");
        eq_ctrl_uart_send_error(errbuf);
    }
}

void eq_ctrl_uart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init_struct;

    memset(&s_eq_uart_handle, 0, sizeof(s_eq_uart_handle));
    memset(s_rx_line, 0, sizeof(s_rx_line));
    memset(s_ready_line, 0, sizeof(s_ready_line));
    s_rx_line_len = 0U;
    s_rx_line_ready = 0U;

    EQ_UART_CLK_ENABLE();
    EQ_UART_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = EQ_UART_TX_PIN | EQ_UART_RX_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = EQ_UART_GPIO_AF;
    HAL_GPIO_Init(EQ_UART_GPIO_PORT, &gpio_init_struct);

    s_eq_uart_handle.Instance = EQ_UART_INSTANCE;
    s_eq_uart_handle.Init.BaudRate = baudrate;
    s_eq_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    s_eq_uart_handle.Init.StopBits = UART_STOPBITS_1;
    s_eq_uart_handle.Init.Parity = UART_PARITY_NONE;
    s_eq_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_eq_uart_handle.Init.Mode = UART_MODE_TX_RX;
    s_eq_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&s_eq_uart_handle);

    __HAL_UART_ENABLE_IT(&s_eq_uart_handle, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&s_eq_uart_handle, UART_IT_IDLE);  /* IDLE detect for line timeout */
    HAL_NVIC_SetPriority(EQ_UART_IRQn, 4, 0);  /* 高于FreeRTOS临界区屏蔽，ISR内不调用FreeRTOS API */
    HAL_NVIC_EnableIRQ(EQ_UART_IRQn);

    eq_ctrl_uart_send_text("EQ UART READY\r\n");
}

void eq_ctrl_uart_poll(void)
{
    if (s_rx_line_ready == 0U)
    {
        return;
    }

    taskENTER_CRITICAL();
    strncpy(s_ready_line, s_rx_line, sizeof(s_ready_line) - 1U);
    s_ready_line[sizeof(s_ready_line) - 1U] = '\0';
    s_rx_line_ready = 0U;
    taskEXIT_CRITICAL();

    eq_ctrl_uart_handle_line(s_ready_line);
}

void eq_ctrl_uart_send_text(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    HAL_UART_Transmit(&s_eq_uart_handle, (uint8_t *)text, (uint16_t)strlen(text), 100U);
}

void USART2_IRQHandler(void)
{
    uint8_t data;

    /* Clear overrun error to prevent UART lockup */
    if (__HAL_UART_GET_FLAG(&s_eq_uart_handle, UART_FLAG_ORE) != RESET)
    {
        __HAL_UART_CLEAR_OREFLAG(&s_eq_uart_handle);
    }

    /* IDLE line detected: treat as end-of-line if bytes accumulated */
    if (__HAL_UART_GET_FLAG(&s_eq_uart_handle, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&s_eq_uart_handle);

        if ((s_rx_line_len > 0U) && (s_rx_line_ready == 0U))
        {
            s_rx_line[s_rx_line_len] = '\0';
            s_rx_line_ready = 1U;
            s_rx_line_len = 0U;
        }
        return;
    }

    if (__HAL_UART_GET_FLAG(&s_eq_uart_handle, UART_FLAG_RXNE) != RESET)
    {
        data = (uint8_t)(s_eq_uart_handle.Instance->DR & 0xFFU);

        if ((data == '\r') || (data == '\0'))
        {
            return;
        }

        if (data == '\n')
        {
            if ((s_rx_line_len > 0U) && (s_rx_line_ready == 0U))
            {
                s_rx_line[s_rx_line_len] = '\0';
                s_rx_line_ready = 1U;
            }
            s_rx_line_len = 0U;
            return;
        }

        /* Only accumulate when previous line has been consumed */
        if (s_rx_line_ready == 0U)
        {
            if (s_rx_line_len < (EQ_CTRL_LINE_MAX_LEN - 1U))
            {
                s_rx_line[s_rx_line_len++] = (char)data;
            }
            else
            {
                s_rx_line_len = 0U;
            }
        }
    }
}