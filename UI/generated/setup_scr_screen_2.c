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

    //Write codes screen_2_label_cmd2 (BAND header)
    ui->screen_2_label_cmd2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_cmd2, "BAND");
    lv_label_set_long_mode(ui->screen_2_label_cmd2, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->screen_2_label_cmd2, 14, 116);
    lv_obj_set_size(ui->screen_2_label_cmd2, 40, 16);

    //Write codes screen_2_label_status (FREQ header)
    ui->screen_2_label_status = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_status, "FREQ");
    lv_obj_set_pos(ui->screen_2_label_status, 54, 116);
    lv_obj_set_size(ui->screen_2_label_status, 42, 16);

    //Write codes screen_2_label_cmd3 (GAIN header)
    ui->screen_2_label_cmd3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_cmd3, "GAIN");
    lv_label_set_long_mode(ui->screen_2_label_cmd3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_cmd3, 106, 116);
    lv_obj_set_size(ui->screen_2_label_cmd3, 44, 16);

    //Write codes screen_2_label_q_header (Q header)
    ui->screen_2_label_q_header = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_q_header, "Q");
    lv_obj_set_pos(ui->screen_2_label_q_header, 160, 116);
    lv_obj_set_size(ui->screen_2_label_q_header, 48, 16);

    //Write codes screen_2_label_band_1
    ui->screen_2_label_band_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_1, "B1");
    lv_obj_set_pos(ui->screen_2_label_band_1, 14, 144);
    lv_obj_set_size(ui->screen_2_label_band_1, 28, 18);

    //Write codes screen_2_label_band_2
    ui->screen_2_label_band_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_2, "B2");
    lv_obj_set_pos(ui->screen_2_label_band_2, 14, 170);
    lv_obj_set_size(ui->screen_2_label_band_2, 28, 18);

    //Write codes screen_2_label_band_3
    ui->screen_2_label_band_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_3, "B3");
    lv_obj_set_pos(ui->screen_2_label_band_3, 14, 196);
    lv_obj_set_size(ui->screen_2_label_band_3, 28, 18);

    //Write codes screen_2_label_band_4
    ui->screen_2_label_band_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_4, "B4");
    lv_obj_set_pos(ui->screen_2_label_band_4, 14, 222);
    lv_obj_set_size(ui->screen_2_label_band_4, 28, 18);

    //Write codes screen_2_label_band_5
    ui->screen_2_label_band_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_band_5, "B5");
    lv_obj_set_pos(ui->screen_2_label_band_5, 14, 248);
    lv_obj_set_size(ui->screen_2_label_band_5, 28, 18);

    //Write codes screen_2_label_freq_1
    ui->screen_2_label_freq_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_1, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_1, 42, 144);
    lv_obj_set_size(ui->screen_2_label_freq_1, 54, 18);

    //Write codes screen_2_label_freq_2
    ui->screen_2_label_freq_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_2, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_2, 42, 170);
    lv_obj_set_size(ui->screen_2_label_freq_2, 54, 18);

    //Write codes screen_2_label_freq_3
    ui->screen_2_label_freq_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_3, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_3, 42, 196);
    lv_obj_set_size(ui->screen_2_label_freq_3, 54, 18);

    //Write codes screen_2_label_freq_4
    ui->screen_2_label_freq_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_4, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_4, 42, 222);
    lv_obj_set_size(ui->screen_2_label_freq_4, 54, 18);

    //Write codes screen_2_label_freq_5
    ui->screen_2_label_freq_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_freq_5, "----Hz");
    lv_obj_set_pos(ui->screen_2_label_freq_5, 42, 248);
    lv_obj_set_size(ui->screen_2_label_freq_5, 54, 18);

    //Write codes screen_2_label_gain_1
    ui->screen_2_label_gain_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_1, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_1, 100, 144);
    lv_obj_set_size(ui->screen_2_label_gain_1, 56, 18);

    //Write codes screen_2_label_gain_2
    ui->screen_2_label_gain_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_2, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_2, 100, 170);
    lv_obj_set_size(ui->screen_2_label_gain_2, 56, 18);

    //Write codes screen_2_label_gain_3
    ui->screen_2_label_gain_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_3, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_3, 100, 196);
    lv_obj_set_size(ui->screen_2_label_gain_3, 56, 18);

    //Write codes screen_2_label_gain_4
    ui->screen_2_label_gain_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_4, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_4, 100, 222);
    lv_obj_set_size(ui->screen_2_label_gain_4, 56, 18);

    //Write codes screen_2_label_gain_5
    ui->screen_2_label_gain_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_gain_5, "+0.0dB");
    lv_obj_set_pos(ui->screen_2_label_gain_5, 100, 248);
    lv_obj_set_size(ui->screen_2_label_gain_5, 56, 18);

    //Write codes screen_2 Q value labels
    ui->screen_2_label_q_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_q_1, "1.00");
    lv_obj_set_pos(ui->screen_2_label_q_1, 160, 144);
    lv_obj_set_size(ui->screen_2_label_q_1, 48, 18);

    ui->screen_2_label_q_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_q_2, "1.00");
    lv_obj_set_pos(ui->screen_2_label_q_2, 160, 170);
    lv_obj_set_size(ui->screen_2_label_q_2, 48, 18);

    ui->screen_2_label_q_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_q_3, "1.00");
    lv_obj_set_pos(ui->screen_2_label_q_3, 160, 196);
    lv_obj_set_size(ui->screen_2_label_q_3, 48, 18);

    ui->screen_2_label_q_4 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_q_4, "1.00");
    lv_obj_set_pos(ui->screen_2_label_q_4, 160, 222);
    lv_obj_set_size(ui->screen_2_label_q_4, 48, 18);

    ui->screen_2_label_q_5 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_q_5, "1.00");
    lv_obj_set_pos(ui->screen_2_label_q_5, 160, 248);
    lv_obj_set_size(ui->screen_2_label_q_5, 48, 18);

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
    lv_obj_set_style_text_color(ui->screen_2_label_q_header, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_q_header, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_q_1, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_q_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_q_1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_q_2, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_q_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_q_2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_q_3, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_q_3, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_q_3, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_q_4, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_q_4, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_q_4, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_q_5, lv_color_hex(0x2d241d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_q_5, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_q_5, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_hint, lv_color_hex(0x5f3b20), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_hint, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_2.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_2);

}
