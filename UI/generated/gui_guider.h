/*
 * @Author: oukaa 3328236081@qq.com
 * @Date: 2026-03-04 20:15:38
 * @LastEditors: oukaa 3328236081@qq.com
 * @LastEditTime: 2026-03-05 20:19:31
 * @FilePath: \Mood_1.0\UI\generated\gui_guider.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
  
	lv_obj_t *screen;
	bool screen_del;
		lv_obj_t *screen_label_1;
		lv_obj_t *screen_label_2;
	lv_obj_t *screen_1;
	bool screen_1_del;
	lv_obj_t *screen_1_img_1;
	lv_obj_t *screen_1_bar_1;
	lv_obj_t *screen_1_label_1;
	lv_obj_t *screen_1_label_2;
	lv_obj_t *screen_1_label_3;
	lv_obj_t *screen_1_btn_1;
	lv_obj_t *screen_1_btn_1_label;
	lv_obj_t *screen_1_btn_2;
	lv_obj_t *screen_1_btn_2_label;
	lv_obj_t *screen_1_btn_3;
	lv_obj_t *screen_1_btn_3_label;
	lv_obj_t *screen_1_chart_1;
	lv_chart_series_t *screen_1_chart_1_0;
	lv_obj_t *screen_1_vol_cont;    /* 音量弹出容器 */
	lv_obj_t *screen_1_vol_bar;     /* 音量进度条 */
	lv_obj_t *screen_1_vol_label;   /* 音量数值标签 */
	lv_obj_t *screen_2;
	bool screen_2_del;
		lv_obj_t *screen_2_label_title;
		lv_obj_t *screen_2_label_link;
		lv_obj_t *screen_2_label_cmd1;
		lv_obj_t *screen_2_label_cmd2;
		lv_obj_t *screen_2_label_cmd3;
		lv_obj_t *screen_2_label_status;
		lv_obj_t *screen_2_label_band_1;
		lv_obj_t *screen_2_label_band_2;
		lv_obj_t *screen_2_label_band_3;
		lv_obj_t *screen_2_label_band_4;
		lv_obj_t *screen_2_label_band_5;
		lv_obj_t *screen_2_label_freq_1;
		lv_obj_t *screen_2_label_freq_2;
		lv_obj_t *screen_2_label_freq_3;
		lv_obj_t *screen_2_label_freq_4;
		lv_obj_t *screen_2_label_freq_5;
		lv_obj_t *screen_2_label_gain_1;
		lv_obj_t *screen_2_label_gain_2;
		lv_obj_t *screen_2_label_gain_3;
		lv_obj_t *screen_2_label_gain_4;
		lv_obj_t *screen_2_label_gain_5;
		lv_obj_t *screen_2_label_hint;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, int32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_screen(lv_ui *ui);
void setup_scr_screen_1(lv_ui *ui);
void setup_scr_screen_2(lv_ui *ui);
LV_IMG_DECLARE(_hour_needle_white_alpha_70x8);
LV_IMG_DECLARE(_min_needle_white_alpha_105x8);
LV_IMG_DECLARE(_second_needle_2_alpha_130x5);
LV_IMG_DECLARE(_5_131x125);

LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_Acme_Regular_22)
LV_FONT_DECLARE(lv_font_ArchitectsDaughter_20)
LV_FONT_DECLARE(lv_font_AlexBrush_Regular_40)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_20)
LV_FONT_DECLARE(lv_font_montserratMedium_18)
LV_FONT_DECLARE(lv_font_song_20)


#ifdef __cplusplus
}
#endif
#endif
