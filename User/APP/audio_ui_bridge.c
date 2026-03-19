/**
 ****************************************************************************************************
 * @file        audio_ui_bridge.c
 * @brief       音频任务与UI任务之间的共享状态桥接模块
 * @note        audio任务写入状态，LVGL任务读取并更新界面
 ****************************************************************************************************
 */

#include "audio_ui_bridge.h"
#include "audio_eq.h"
#include "eq_ctrl_uart.h"
#include "recorder.h"
#include "lvgl.h"
#include "gui_guider.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 全局共享状态 */
audio_ui_state_t g_audio_ui = {0};

/* 前向声明 */
void audio_ui_update_recorder(void);

static void audio_ui_load_page(uint8_t page)
{
    lv_obj_t *target = NULL;

    switch (page)
    {
    case UI_PAGE_PLAYER:
        target = guider_ui.screen_1;
        break;
    case UI_PAGE_EQ:
        target = guider_ui.screen_2;
        break;
    case UI_PAGE_RECORDER:
        target = guider_ui.screen_3;
        break;
    case UI_PAGE_LOADING:
    default:
        target = guider_ui.screen;
        page = UI_PAGE_LOADING;
        break;
    }

    if (target == NULL)
    {
        return;
    }

    if (lv_scr_act() != target)
    {
        lv_scr_load(target);
    }

    g_audio_ui.current_page = page;
}

static void audio_ui_update_eq_page(void)
{
    static uint8_t refresh_divider = 0U;
    audio_eq_status_t status;
    char line[64];
    lv_obj_t *band_labels[AUDIO_EQ_BAND_COUNT];
    lv_obj_t *freq_labels[AUDIO_EQ_BAND_COUNT];
    lv_obj_t *gain_labels[AUDIO_EQ_BAND_COUNT];
    lv_obj_t *q_labels[AUDIO_EQ_BAND_COUNT];
    uint8_t band;

    if (guider_ui.screen_2 == NULL)
    {
        return;
    }

    if (++refresh_divider < 10U)
    {
        return;
    }
    refresh_divider = 0U;

    audio_eq_get_status(&status);
    if ((g_audio_ui.current_page != UI_PAGE_EQ) &&
        (g_audio_ui.eq_changed == 0U) &&
        (g_audio_ui.vol_changed == 0U))
    {
        return;
    }
    g_audio_ui.eq_changed = 0U;

    snprintf(line, sizeof(line), "EQ   %s", status.enabled ? "ON" : "OFF");
    lv_label_set_text(guider_ui.screen_2_label_link, line);

    snprintf(line, sizeof(line), "VOL  %u", (unsigned int)g_audio_ui.vol_level);
    lv_label_set_text(guider_ui.screen_2_label_cmd1, line);

    band_labels[0] = guider_ui.screen_2_label_band_1;
    band_labels[1] = guider_ui.screen_2_label_band_2;
    band_labels[2] = guider_ui.screen_2_label_band_3;
    band_labels[3] = guider_ui.screen_2_label_band_4;
    band_labels[4] = guider_ui.screen_2_label_band_5;
    freq_labels[0] = guider_ui.screen_2_label_freq_1;
    freq_labels[1] = guider_ui.screen_2_label_freq_2;
    freq_labels[2] = guider_ui.screen_2_label_freq_3;
    freq_labels[3] = guider_ui.screen_2_label_freq_4;
    freq_labels[4] = guider_ui.screen_2_label_freq_5;
    gain_labels[0] = guider_ui.screen_2_label_gain_1;
    gain_labels[1] = guider_ui.screen_2_label_gain_2;
    gain_labels[2] = guider_ui.screen_2_label_gain_3;
    gain_labels[3] = guider_ui.screen_2_label_gain_4;
    gain_labels[4] = guider_ui.screen_2_label_gain_5;
    q_labels[0] = guider_ui.screen_2_label_q_1;
    q_labels[1] = guider_ui.screen_2_label_q_2;
    q_labels[2] = guider_ui.screen_2_label_q_3;
    q_labels[3] = guider_ui.screen_2_label_q_4;
    q_labels[4] = guider_ui.screen_2_label_q_5;

    for (band = 0U; band < AUDIO_EQ_BAND_COUNT; band++)
    {
        snprintf(line, sizeof(line), "B%u", (unsigned int)(band + 1U));
        lv_label_set_text(band_labels[band], line);
        snprintf(line, sizeof(line), "%uHz", (unsigned int)status.freq_hz[band]);
        lv_label_set_text(freq_labels[band], line);
        snprintf(line,
                 sizeof(line),
                 "%c%d.%ddB",
                 status.gain_db_x10[band] >= 0 ? '+' : '-',
                 abs((int)status.gain_db_x10[band] / 10),
                 (int)abs(status.gain_db_x10[band] % 10));
        lv_label_set_text(gain_labels[band], line);
        snprintf(line, sizeof(line), "%u.%02u",
                 (unsigned int)(status.q_x100[band] / 100U),
                 (unsigned int)(status.q_x100[band] % 100U));
        lv_label_set_text(q_labels[band], line);
    }
}

/* ==================== Audio任务调用的接口 ==================== */

void audio_ui_set_song_name(const char *name)
{
    /* 复制歌曲名，截断到缓冲区大小 */
    strncpy(g_audio_ui.song_name, name, sizeof(g_audio_ui.song_name) - 1);
    g_audio_ui.song_name[sizeof(g_audio_ui.song_name) - 1] = '\0';
    g_audio_ui.song_changed = 1;
    if (g_audio_ui.loading_done == 0U)
    {
        g_audio_ui.loading_done = 1U;
        g_audio_ui.requested_page = UI_PAGE_PLAYER;
        g_audio_ui.page_change_pending = 1U;
    }
    g_audio_ui.update_flag = 1;
    eq_ctrl_uart_send_player_status();
}

void audio_ui_set_time(uint32_t cur, uint32_t total, uint32_t bitrate)
{
    g_audio_ui.cur_time = cur;
    g_audio_ui.total_time = total;
    g_audio_ui.bitrate = bitrate;
    g_audio_ui.update_flag = 1;
}

void audio_ui_set_playing(uint8_t playing)
{
    g_audio_ui.is_playing = playing;
    g_audio_ui.update_flag = 1;
    eq_ctrl_uart_send_player_status();
}

void audio_ui_set_index(uint16_t index, uint16_t total)
{
    g_audio_ui.cur_index = index;
    g_audio_ui.total_files = total;
    g_audio_ui.update_flag = 1;
}

void audio_ui_set_btn_pressed(uint8_t btn)
{
    switch (btn) {
        case 0: g_audio_ui.btn_prev_pressed = 1; break;
        case 1: g_audio_ui.btn_next_pressed = 1; break;
        case 2: g_audio_ui.btn_pause_pressed = 1; break;
    }
    g_audio_ui.btn_press_timer = 10;  /* 按下效果持续约100ms(10次LVGL更新) */
    g_audio_ui.update_flag = 1;
}

void audio_ui_set_spectrum(const uint8_t *bars)
{
    memcpy(g_audio_ui.spectrum_bars, bars, 15);
    g_audio_ui.spectrum_updated = 1;
    g_audio_ui.update_flag = 1;
}

void audio_ui_set_volume(uint8_t level)
{
    g_audio_ui.vol_level = level;
    g_audio_ui.vol_changed = 1;
    g_audio_ui.vol_show_timer = 200U;  /* 显示2秒 (200 × 10ms LVGL周期) */
    g_audio_ui.update_flag = 1;
}

void audio_ui_notify_eq_changed(void)
{
    g_audio_ui.eq_changed = 1U;
    g_audio_ui.update_flag = 1U;
}

void audio_ui_request_page_toggle(void)
{
    if (g_audio_ui.loading_done == 0U)
    {
        return;
    }

    if (g_audio_ui.current_page == UI_PAGE_EQ)
    {
        g_audio_ui.requested_page = UI_PAGE_PLAYER;
    }
    else if (g_audio_ui.current_page == UI_PAGE_RECORDER)
    {
        /* 从录音页面不能通过普通切换退出 */
        return;
    }
    else
    {
        g_audio_ui.requested_page = UI_PAGE_EQ;
    }

    g_audio_ui.page_change_pending = 1U;
    g_audio_ui.update_flag = 1U;
}

/* ==================== UI任务调用的接口 ==================== */

/**
 * @brief  初始化播放器界面（screen_1）并加载
 */
void audio_ui_init_screen(void)
{
    memset(&g_audio_ui, 0, sizeof(g_audio_ui));

    /* 创建启动页、播放器页、EQ页和录音页 */
    setup_scr_screen(&guider_ui);
    setup_scr_screen_1(&guider_ui);
    setup_scr_screen_2(&guider_ui);
    setup_scr_screen_3(&guider_ui);
    
    /* 首屏显示加载页 */
    g_audio_ui.current_page = UI_PAGE_LOADING;
    g_audio_ui.requested_page = UI_PAGE_LOADING;
    audio_ui_load_page(UI_PAGE_LOADING);
    
    /* 初始化进度条为0 */
    lv_bar_set_value(guider_ui.screen_1_bar_1, 0, LV_ANIM_OFF);
    
    /* 初始化时间标签 */
    lv_label_set_text(guider_ui.screen_1_label_1, "0:00");
    lv_label_set_text(guider_ui.screen_1_label_2, "0:00");
    
    /* 初始化歌名 */
    lv_label_set_text(guider_ui.screen_1_label_3, "Loading...");

    /* 页面切换提示 */
    lv_label_set_text(guider_ui.screen_2_label_link, "EQ   --");
    lv_label_set_text(guider_ui.screen_2_label_cmd1, "VOL  --");
    lv_label_set_text(guider_ui.screen_2_label_band_1, "B1");
    lv_label_set_text(guider_ui.screen_2_label_band_2, "B2");
    lv_label_set_text(guider_ui.screen_2_label_band_3, "B3");
    lv_label_set_text(guider_ui.screen_2_label_band_4, "B4");
    lv_label_set_text(guider_ui.screen_2_label_band_5, "B5");
    lv_label_set_text(guider_ui.screen_2_label_freq_1, "----Hz");
    lv_label_set_text(guider_ui.screen_2_label_freq_2, "----Hz");
    lv_label_set_text(guider_ui.screen_2_label_freq_3, "----Hz");
    lv_label_set_text(guider_ui.screen_2_label_freq_4, "----Hz");
    lv_label_set_text(guider_ui.screen_2_label_freq_5, "----Hz");
    lv_label_set_text(guider_ui.screen_2_label_gain_1, "+0.0dB");
    lv_label_set_text(guider_ui.screen_2_label_gain_2, "+0.0dB");
    lv_label_set_text(guider_ui.screen_2_label_gain_3, "+0.0dB");
    lv_label_set_text(guider_ui.screen_2_label_gain_4, "+0.0dB");
    lv_label_set_text(guider_ui.screen_2_label_gain_5, "+0.0dB");
    
    /* 初始状态：暂停 → 显示图片，隐藏图表 */
    lv_obj_clear_flag(guider_ui.screen_1_img_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(guider_ui.screen_1_chart_1, LV_OBJ_FLAG_HIDDEN);
    
    /* 播放按钮显示PLAY符号 */
    lv_label_set_text(guider_ui.screen_1_btn_3_label, LV_SYMBOL_PLAY);
    
    /* ====== 音量弹出条 ====== */
    {
        /* 半透明容器，居中偏上 */
        lv_obj_t *cont = lv_obj_create(guider_ui.screen_1);
        lv_obj_set_size(cont, 180, 50);
        lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 90);
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(cont, LV_OPA_90, 0);
        lv_obj_set_style_radius(cont, 10, 0);
        lv_obj_set_style_border_width(cont, 1, 0);
        lv_obj_set_style_border_color(cont, lv_color_hex(0x888888), 0);
        lv_obj_set_style_pad_all(cont, 5, 0);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        
        /* 音量图标 + 数字标签 */
        lv_obj_t *lbl = lv_label_create(cont);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_label_set_text(lbl, LV_SYMBOL_VOLUME_MAX " 50");
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);
        
        /* 进度条 */
        lv_obj_t *bar = lv_bar_create(cont);
        lv_obj_set_size(bar, 160, 10);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, 50, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x444444), LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x2195f6), LV_PART_INDICATOR);
        
        guider_ui.screen_1_vol_cont = cont;
        guider_ui.screen_1_vol_bar = bar;
        guider_ui.screen_1_vol_label = lbl;
        
        /* 初始隐藏 */
        lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief  LVGL任务中周期调用，将共享状态更新到LVGL控件
 */
void audio_ui_update_lvgl(void)
{
    static uint8_t last_playing = 0xFF;  /* 用于检测播放状态变化 */
    static uint32_t last_cur_time = 0xFFFFFFFF;
    char buf[16];
    
    /* 确保screen_1已创建 */
    if (guider_ui.screen_1 == NULL)
        return;
    
    if (g_audio_ui.page_change_pending)
    {
        audio_ui_load_page(g_audio_ui.requested_page);
        g_audio_ui.page_change_pending = 0U;
    }

    /* 处理按钮按下的视觉反馈（独立于update_flag，每次都检查） */
    if (g_audio_ui.btn_press_timer > 0)
    {
        /* 设置按下状态 */
        if (g_audio_ui.btn_prev_pressed)
            lv_obj_add_state(guider_ui.screen_1_btn_1, LV_STATE_PRESSED);
        if (g_audio_ui.btn_next_pressed)
            lv_obj_add_state(guider_ui.screen_1_btn_2, LV_STATE_PRESSED);
        if (g_audio_ui.btn_pause_pressed)
            lv_obj_add_state(guider_ui.screen_1_btn_3, LV_STATE_PRESSED);
        
        g_audio_ui.btn_press_timer--;
        
        /* 计时结束，清除按下状态 */
        if (g_audio_ui.btn_press_timer == 0)
        {
            g_audio_ui.btn_prev_pressed = 0;
            g_audio_ui.btn_next_pressed = 0;
            g_audio_ui.btn_pause_pressed = 0;
            lv_obj_clear_state(guider_ui.screen_1_btn_1, LV_STATE_PRESSED);
            lv_obj_clear_state(guider_ui.screen_1_btn_2, LV_STATE_PRESSED);
            lv_obj_clear_state(guider_ui.screen_1_btn_3, LV_STATE_PRESSED);
        }
    }
    
    /* ====== 音量弹出条（独立于update_flag，每10ms周期运行） ====== */
    if (guider_ui.screen_1_vol_cont != NULL)
    {
        if (g_audio_ui.vol_changed)
        {
            char vbuf[24];
            const char *icon;
            
            g_audio_ui.vol_changed = 0;
            
            /* 根据音量选图标 */
            if (g_audio_ui.vol_level == 0)
                icon = LV_SYMBOL_MUTE;
            else if (g_audio_ui.vol_level < 50)
                icon = LV_SYMBOL_VOLUME_MID;
            else
                icon = LV_SYMBOL_VOLUME_MAX;
            
            snprintf(vbuf, sizeof(vbuf), "%s %d", icon, g_audio_ui.vol_level);
            lv_label_set_text(guider_ui.screen_1_vol_label, vbuf);
            lv_bar_set_value(guider_ui.screen_1_vol_bar, g_audio_ui.vol_level, LV_ANIM_ON);
            
            /* 显示弹出条并确保在最顶层 */
            lv_obj_clear_flag(guider_ui.screen_1_vol_cont, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(guider_ui.screen_1_vol_cont);
        }
        
        /* 倒计时隐藏 */
        if (g_audio_ui.vol_show_timer > 0U)
        {
            g_audio_ui.vol_show_timer--;
            if (g_audio_ui.vol_show_timer == 0U)
            {
                lv_obj_add_flag(guider_ui.screen_1_vol_cont, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    audio_ui_update_eq_page();
    audio_ui_update_recorder();
    
    if (!g_audio_ui.update_flag)
        return;
    
    /* 更新歌曲名 */
    if (g_audio_ui.song_changed)
    {
        /* 使用中日韩英字体显示歌名 */
        lv_obj_set_style_text_font(guider_ui.screen_1_label_3, &lv_font_song_20, 0);
        lv_label_set_text(guider_ui.screen_1_label_3, g_audio_ui.song_name);
        g_audio_ui.song_changed = 0;
    }
    
    /* 更新播放时间 */
    if (last_cur_time != g_audio_ui.cur_time)
    {
        last_cur_time = g_audio_ui.cur_time;
        
        /* 当前时间 */
        snprintf(buf, sizeof(buf), "%d:%02d", 
                 (int)(g_audio_ui.cur_time / 60), 
                 (int)(g_audio_ui.cur_time % 60));
        lv_label_set_text(guider_ui.screen_1_label_1, buf);
        
        /* 总时间 */
        snprintf(buf, sizeof(buf), "%d:%02d", 
                 (int)(g_audio_ui.total_time / 60), 
                 (int)(g_audio_ui.total_time % 60));
        lv_label_set_text(guider_ui.screen_1_label_2, buf);
        
        /* 进度条 (0~100) */
        if (g_audio_ui.total_time > 0)
        {
            int32_t progress = (int32_t)(g_audio_ui.cur_time * 100 / g_audio_ui.total_time);
            if (progress > 100) progress = 100;
            lv_bar_set_value(guider_ui.screen_1_bar_1, progress, LV_ANIM_ON);
        }
        else
        {
            lv_bar_set_value(guider_ui.screen_1_bar_1, 0, LV_ANIM_OFF);
        }
    }
    
    /* 播放/暂停状态变化 → 切换图片/图表显示 + 按钮图标 */
    if (last_playing != g_audio_ui.is_playing)
    {
        last_playing = g_audio_ui.is_playing;
        
        if (g_audio_ui.is_playing)
        {
            /* 播放中：显示图表，隐藏图片 */
            lv_obj_add_flag(guider_ui.screen_1_img_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(guider_ui.screen_1_chart_1, LV_OBJ_FLAG_HIDDEN);
            /* 按钮显示暂停符号 */
            lv_label_set_text(guider_ui.screen_1_btn_3_label, LV_SYMBOL_PAUSE);
        }
        else
        {
            /* 暂停中：显示图片，隐藏图表 */
            lv_obj_clear_flag(guider_ui.screen_1_img_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(guider_ui.screen_1_chart_1, LV_OBJ_FLAG_HIDDEN);
            /* 按钮显示播放符号 */
            lv_label_set_text(guider_ui.screen_1_btn_3_label, LV_SYMBOL_PLAY);
        }
    }
    
    /* 实时频谱更新 → 写入图表数据 */
    if (g_audio_ui.spectrum_updated && g_audio_ui.is_playing)
    {
        lv_chart_series_t *ser = guider_ui.screen_1_chart_1_0;
        if (ser && ser->y_points)
        {
            for (int i = 0; i < 15; i++)
            {
                ser->y_points[i] = (lv_coord_t)g_audio_ui.spectrum_bars[i];
            }
            lv_chart_refresh(guider_ui.screen_1_chart_1);
        }
        g_audio_ui.spectrum_updated = 0;
    }
    
    g_audio_ui.update_flag = 0;
}

/* ==================== 录音相关接口 ==================== */

void audio_ui_set_rec_state(uint8_t state)
{
    g_audio_ui.rec_state = state;
    g_audio_ui.rec_updated = 1;
    g_audio_ui.update_flag = 1;
}

void audio_ui_set_rec_time(uint32_t duration, uint32_t size)
{
    g_audio_ui.rec_duration = duration;
    g_audio_ui.rec_size = size;
    g_audio_ui.rec_updated = 1;
    g_audio_ui.update_flag = 1;
}

void audio_ui_request_enter_recorder(void)
{
    g_audio_ui.rec_enter_request = 1;
    g_audio_ui.requested_page = UI_PAGE_RECORDER;
    g_audio_ui.page_change_pending = 1;
    g_audio_ui.update_flag = 1;
}

void audio_ui_request_exit_recorder(void)
{
    g_audio_ui.rec_exit_request = 1;
    g_audio_ui.requested_page = UI_PAGE_PLAYER;
    g_audio_ui.page_change_pending = 1;
    g_audio_ui.update_flag = 1;
}

/**
 * @brief  录音界面(screen_3)定时更新 - 在 audio_ui_update_lvgl 结尾调用
 */
void audio_ui_update_recorder(void)
{
    char buf[32];

    if (guider_ui.screen_3 == NULL)
        return;
    if (g_audio_ui.current_page != UI_PAGE_RECORDER)
        return;

    if (!g_audio_ui.rec_updated)
        return;
    g_audio_ui.rec_updated = 0;

    /* 更新录音时长 */
    {
        uint32_t d = g_audio_ui.rec_duration;
        snprintf(buf, sizeof(buf), "%02d:%02d", (int)(d / 60), (int)(d % 60));
        lv_label_set_text(guider_ui.screen_3_label_time, buf);
    }

    /* 更新文件大小 */
    {
        uint32_t kb = g_audio_ui.rec_size / 1024;
        if (kb < 1024)
            snprintf(buf, sizeof(buf), "%lu KB", (unsigned long)kb);
        else
            snprintf(buf, sizeof(buf), "%lu.%lu MB", (unsigned long)(kb / 1024), (unsigned long)((kb % 1024) * 10 / 1024));
        lv_label_set_text(guider_ui.screen_3_label_size, buf);
    }

    /* 更新状态 */
    switch (g_audio_ui.rec_state)
    {
    case 0: /* IDLE */
        lv_label_set_text(guider_ui.screen_3_label_status, "Ready");
        lv_label_set_text(guider_ui.screen_3_btn_rec_label, LV_SYMBOL_PLAY);
        lv_obj_set_style_text_color(guider_ui.screen_3_label_led, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(guider_ui.screen_3_label_led, LV_SYMBOL_STOP);
        break;
    case 1: /* RECORDING */
        lv_label_set_text(guider_ui.screen_3_label_status, "Recording...");
        lv_label_set_text(guider_ui.screen_3_btn_rec_label, LV_SYMBOL_STOP);
        lv_obj_set_style_text_color(guider_ui.screen_3_label_led, lv_color_hex(0xff4444), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(guider_ui.screen_3_label_led, LV_SYMBOL_STOP);
        break;
    case 2: /* PAUSED */
        lv_label_set_text(guider_ui.screen_3_label_status, "Paused");
        lv_label_set_text(guider_ui.screen_3_btn_rec_label, LV_SYMBOL_PLAY);
        lv_obj_set_style_text_color(guider_ui.screen_3_label_led, lv_color_hex(0xffaa00), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(guider_ui.screen_3_label_led, LV_SYMBOL_PAUSE);
        break;
    case 3: /* SAVING */
        lv_label_set_text(guider_ui.screen_3_label_status, "Saving...");
        break;
    case 4: /* DONE */
        lv_label_set_text(guider_ui.screen_3_label_status, "Saved!");
        lv_label_set_text(guider_ui.screen_3_btn_rec_label, LV_SYMBOL_PLAY);
        lv_obj_set_style_text_color(guider_ui.screen_3_label_led, lv_color_hex(0x44ff44), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(guider_ui.screen_3_label_led, LV_SYMBOL_OK);
        break;
    }
}
