/**
 ****************************************************************************************************
 * @file        audio_ui_bridge.h
 * @brief       音频任务与UI任务之间的共享状态桥接模块
 * @note        audio任务写入状态，LVGL任务读取并更新界面
 ****************************************************************************************************
 */

#ifndef __AUDIO_UI_BRIDGE_H
#define __AUDIO_UI_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

/* 从UI发送到Audio的命令 */
#define UI_CMD_NONE         0
#define UI_CMD_NEXT         1
#define UI_CMD_PREV         2
#define UI_CMD_PAUSE        3
#define UI_CMD_PLAY         4

#define UI_PAGE_LOADING     0
#define UI_PAGE_PLAYER      1
#define UI_PAGE_EQ          2
#define UI_PAGE_RECORDER    3

/* 音频-UI 共享状态结构体 */
typedef struct {
    /* === Audio任务写入，UI任务读取 === */
    char     song_name[64];     /* 当前歌曲名 */
    uint32_t cur_time;          /* 当前播放时间（秒） */
    uint32_t total_time;        /* 歌曲总时间（秒） */
    uint32_t bitrate;           /* 比特率 */
    uint16_t cur_index;         /* 当前曲目索引（从1开始）*/
    uint16_t total_files;       /* 总曲目数 */
    uint8_t  is_playing;        /* 1=正在播放, 0=暂停 */
    uint8_t  update_flag;       /* 有新数据需要UI更新时置1 */
    uint8_t  song_changed;      /* 歌曲切换标志，UI读取后清零 */
    
    /* === 按键按下状态（用于按钮视觉反馈） === */
    volatile uint8_t btn_prev_pressed;   /* 上一首按钮按下 */
    volatile uint8_t btn_next_pressed;   /* 下一首按钮按下 */
    volatile uint8_t btn_pause_pressed;  /* 暂停按钮按下 */
    volatile uint8_t btn_press_timer;    /* 按下效果计时器 */
    
    /* === UI任务写入，Audio任务读取 === */
    volatile uint8_t  ui_cmd;   /* UI发送的命令 */
    volatile uint8_t  current_page;       /* 当前页面 */
    volatile uint8_t  requested_page;     /* 请求切换到的页面 */
    volatile uint8_t  page_change_pending;/* 页面切换请求 */
    volatile uint8_t  loading_done;       /* 已完成启动页切换 */
    volatile uint8_t  eq_changed;         /* EQ参数变化标志 */
    
    /* === 实时频谱 (Audio任务写入, UI任务读取) === */
    uint8_t  spectrum_bars[15];      /* 15个频段幅值 0~100 */
    uint8_t  spectrum_updated;       /* 有新频谱数据时置1, UI读取后清0 */
    
    /* === 音量弹出显示 (EQ_UART任务写入, UI任务读取) === */
    volatile uint8_t  vol_level;         /* 当前音量 0-100 */
    volatile uint8_t  vol_changed;       /* 音量变化标志 */
    volatile uint16_t vol_show_timer;    /* 弹出显示倒计时(LVGL周期数) */
    
    /* === 录音状态 (Audio任务写入, UI任务读取) === */
    volatile uint8_t  rec_state;         /* RecorderState_t */
    volatile uint32_t rec_duration;      /* 录音时长(秒) */
    volatile uint32_t rec_size;          /* 已录制大小(字节) */
    volatile uint8_t  rec_updated;       /* 录音状态更新标志 */
    volatile uint8_t  rec_enter_request; /* 请求进入录音界面 */
    volatile uint8_t  rec_exit_request;  /* 请求退出录音界面 */
} audio_ui_state_t;

/* 全局共享状态实例 */
extern audio_ui_state_t g_audio_ui;

/* Audio任务调用的函数 */
void audio_ui_set_song_name(const char *name);
void audio_ui_set_time(uint32_t cur, uint32_t total, uint32_t bitrate);
void audio_ui_set_playing(uint8_t playing);
void audio_ui_set_index(uint16_t index, uint16_t total);
void audio_ui_set_btn_pressed(uint8_t btn);  /* 设置按钮按下状态：0=prev,1=next,2=pause */

/* 频谱数据设置 (Audio任务调用) */
void audio_ui_set_spectrum(const uint8_t *bars);

/* 音量弹出显示 (EQ_UART任务调用) */
void audio_ui_set_volume(uint8_t level);
void audio_ui_notify_eq_changed(void);

/* UI任务调用的函数 */
void audio_ui_update_lvgl(void);
void audio_ui_init_screen(void);
void audio_ui_request_page_toggle(void);

/* 录音相关 */
void audio_ui_set_rec_state(uint8_t state);
void audio_ui_set_rec_time(uint32_t duration, uint32_t size);
void audio_ui_request_enter_recorder(void);
void audio_ui_request_exit_recorder(void);

#endif /* __AUDIO_UI_BRIDGE_H */
