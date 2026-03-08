/*
 * custom.c — 自定义UI逻辑（已重写，移除旧版NXP演示代码）
 * 频谱动画功能待后续实现
 */

#include <stdio.h>
#include "lvgl.h"
#include "custom.h"

void custom_init(lv_ui *ui)
{
    /* 预留：后续可在此添加频谱动画等自定义逻辑 */
}

/* 以下函数保留空实现，避免链接错误 */
void tracks_up(void) {}
void tracks_down(void) {}
bool tracks_is_up(void) { return false; }
void lv_demo_music_resume(void) {}
void lv_demo_music_pause(void) {}
void lv_demo_music_album_next(bool next) { (void)next; }
void lv_demo_music_play(uint32_t id) { (void)id; }
void lv_demo_music_list_btn_check(uint32_t track_id, bool state) { (void)track_id; (void)state; }
