/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

void setup_scr_screen_2(lv_ui *ui)
{
    lv_obj_t *panel_top;
    lv_obj_t *panel_bottom;
    lv_obj_t *line_bottom;

    //Write codes screen_2
    ui->screen_2 = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_2, 240, 320);
    lv_obj_set_scrollbar_mode(ui->screen_2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->screen_2, LV_OBJ_FLAG_SCROLLABLE);

    //Write style for screen_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_2, lv_color_hex(0xf4e2c8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_2, lv_color_hex(0xf8b36a), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->screen_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->screen_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    panel_top = lv_obj_create(ui->screen_2);
    lv_obj_set_pos(panel_top, 12, 48);
    lv_obj_set_size(panel_top, 216, 52);
    lv_obj_clear_flag(panel_top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(panel_top, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel_top, lv_color_hex(0xfffbf5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel_top, 220, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(panel_top, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(panel_top, lv_color_hex(0x9f6a36), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(panel_top, LV_OPA_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    panel_bottom = lv_obj_create(ui->screen_2);
    lv_obj_set_pos(panel_bottom, 12, 108);
    lv_obj_set_size(panel_bottom, 216, 176);
    lv_obj_clear_flag(panel_bottom, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(panel_bottom, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel_bottom, lv_color_hex(0xfffbf5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel_bottom, 224, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_bottom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(panel_bottom, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(panel_bottom, lv_color_hex(0x8b5e34), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(panel_bottom, LV_OPA_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_bottom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    line_bottom = lv_obj_create(panel_bottom);
    lv_obj_set_pos(line_bottom, 10, 28);
    lv_obj_set_size(line_bottom, 196, 1);
    lv_obj_set_style_bg_color(line_bottom, lv_color_hex(0xe7c8a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(line_bottom, LV_OPA_90, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(line_bottom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(line_bottom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_clear_flag(line_bottom, LV_OBJ_FLAG_SCROLLABLE);

    //Write codes screen_2_label_title
    ui->screen_2_label_title = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_title, "设置");
    lv_obj_set_pos(ui->screen_2_label_title, 85, 12);
    lv_obj_set_size(ui->screen_2_label_title, 70, 24);

    //Write style for screen_2_label_title, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_2_label_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_2_label_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_title, lv_color_hex(0x4d2e18), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_title, &lv_font_song_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_label_link
    ui->screen_2_label_link = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_link, "EQ   --");
    lv_label_set_long_mode(ui->screen_2_label_link, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_link, 26, 64);
    lv_obj_set_size(ui->screen_2_label_link, 78, 18);

    //Write codes screen_2_label_cmd1
    ui->screen_2_label_cmd1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_cmd1, "VOL  --");
    lv_label_set_long_mode(ui->screen_2_label_cmd1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_cmd1, 126, 64);
    lv_obj_set_size(ui->screen_2_label_cmd1, 78, 18);

    //Write codes screen_2_label_cmd2
    ui->screen_2_label_cmd2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_cmd2, "BAND");
    lv_label_set_long_mode(ui->screen_2_label_cmd2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_cmd2, 22, 116);
    lv_obj_set_size(ui->screen_2_label_cmd2, 38, 16);

    //Write codes screen_2_label_cmd3
    ui->screen_2_label_cmd3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_cmd3, "GAIN");
    lv_label_set_long_mode(ui->screen_2_label_cmd3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_cmd3, 160, 116);
    lv_obj_set_size(ui->screen_2_label_cmd3, 44, 16);

    //Write codes screen_2_label_status
    ui->screen_2_label_status = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_status, "FREQ");
    lv_obj_set_pos(ui->screen_2_label_status, 92, 116);
    lv_obj_set_size(ui->screen_2_label_status, 44, 16);

    //Write codes screen_2_label_band_1
    ui->screen_2_label_band_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_1, "B1");
    lv_obj_set_pos(ui->screen_2_label_band_1, 20, 144);
    lv_obj_set_size(ui->screen_2_label_band_1, 28, 18);

    //Write codes screen_2_label_band_2
    ui->screen_2_label_band_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_2, "B2");
    lv_obj_set_pos(ui->screen_2_label_band_2, 20, 170);
    lv_obj_set_size(ui->screen_2_label_band_2, 28, 18);

    //Write codes screen_2_label_band_3
    ui->screen_2_label_band_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_3, "B3");
    lv_obj_set_pos(ui->screen_2_label_band_3, 20, 196);
    lv_obj_set_size(ui->screen_2_label_band_3, 28, 18);

    //Write codes screen_2_label_band_4
    ui->screen_2_label_band_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_4, "B4");
    lv_obj_set_pos(ui->screen_2_label_band_4, 20, 222);
    lv_obj_set_size(ui->screen_2_label_band_4, 28, 18);

    //Write codes screen_2_label_band_5
    ui->screen_2_label_band_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_5, "B5");
    lv_obj_set_pos(ui->screen_2_label_band_5, 20, 248);
    lv_obj_set_size(ui->screen_2_label_band_5, 28, 18);

    //Write codes screen_2_label_freq_1
    ui->screen_2_label_freq_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_1, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_1, 72, 144);
    lv_obj_set_size(ui->screen_2_label_freq_1, 64, 18);

    //Write codes screen_2_label_freq_2
    ui->screen_2_label_freq_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_2, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_2, 72, 170);
    lv_obj_set_size(ui->screen_2_label_freq_2, 64, 18);

    //Write codes screen_2_label_freq_3
    ui->screen_2_label_freq_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_3, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_3, 72, 196);
    lv_obj_set_size(ui->screen_2_label_freq_3, 64, 18);

    //Write codes screen_2_label_freq_4
    ui->screen_2_label_freq_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_4, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_4, 72, 222);
    lv_obj_set_size(ui->screen_2_label_freq_4, 64, 18);

    //Write codes screen_2_label_freq_5
    ui->screen_2_label_freq_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_5, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_5, 72, 248);
    lv_obj_set_size(ui->screen_2_label_freq_5, 64, 18);

    //Write codes screen_2_label_gain_1
    ui->screen_2_label_gain_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_1, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_1, 144, 144);
    lv_obj_set_size(ui->screen_2_label_gain_1, 60, 18);

    //Write codes screen_2_label_gain_2
    ui->screen_2_label_gain_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_2, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_2, 144, 170);
    lv_obj_set_size(ui->screen_2_label_gain_2, 60, 18);

    //Write codes screen_2_label_gain_3
    ui->screen_2_label_gain_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_3, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_3, 144, 196);
    lv_obj_set_size(ui->screen_2_label_gain_3, 60, 18);

    //Write codes screen_2_label_gain_4
    ui->screen_2_label_gain_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_4, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_4, 144, 222);
    lv_obj_set_size(ui->screen_2_label_gain_4, 60, 18);

    //Write codes screen_2_label_gain_5
    ui->screen_2_label_gain_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_5, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_5, 144, 248);
    lv_obj_set_size(ui->screen_2_label_gain_5, 60, 18);

    //Write codes screen_2_label_hint
    ui->screen_2_label_hint = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_hint, "");
    lv_obj_set_pos(ui->screen_2_label_hint, 0, 0);
    lv_obj_set_size(ui->screen_2_label_hint, 1, 1);
    lv_obj_add_flag(ui->screen_2_label_hint, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_2 labels, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_2_label_link, lv_color_hex(0x5f3b20), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_link, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_link, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui->screen_2_label_cmd1, lv_color_hex(0x9a5f1a), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_cmd1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_cmd2, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_cmd2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_cmd3, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_cmd3, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_status, lv_color_hex(0x9a5f1a), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_status, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_status, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_band_1, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_band_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_band_2, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_band_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_band_3, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_band_3, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_band_4, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_band_4, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_band_5, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_band_5, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_freq_1, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_freq_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_freq_1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_freq_2, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_freq_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_freq_2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_freq_3, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_freq_3, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_freq_3, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_freq_4, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_freq_4, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_freq_4, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_freq_5, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_freq_5, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_freq_5, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_gain_1, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_gain_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_gain_1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_gain_2, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_gain_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_gain_2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_gain_3, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_gain_3, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_gain_3, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_gain_4, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_gain_4, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_gain_4, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_gain_5, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_gain_5, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_gain_5, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_hint, lv_color_hex(0x5f3b20), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_hint, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_2.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_2);

}
