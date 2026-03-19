/*
 * @brief       录音界面(screen_3) - 显示录音状态、时长及控制按钮
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

void setup_scr_screen_3(lv_ui *ui)
{
    /* 创建录音页面 */
    ui->screen_3 = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_3, 240, 320);
    lv_obj_set_scrollbar_mode(ui->screen_3, LV_SCROLLBAR_MODE_OFF);

    /* 背景: 深色渐变 */
    lv_obj_set_style_bg_opa(ui->screen_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_3, lv_color_hex(0x2d2d3f), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_3, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_3, lv_color_hex(0x1a1a2e), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ========== 标题 "REC" ========== */
    ui->screen_3_label_title = lv_label_create(ui->screen_3);
    lv_label_set_text(ui->screen_3_label_title, LV_SYMBOL_AUDIO " REC");
    lv_obj_set_pos(ui->screen_3_label_title, 0, 20);
    lv_obj_set_width(ui->screen_3_label_title, 240);
    lv_obj_set_style_text_color(ui->screen_3_label_title, lv_color_hex(0xff4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_label_title, &lv_font_montserratMedium_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_3_label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_label_title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_label_title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ========== 录音状态标签 ========== */
    ui->screen_3_label_status = lv_label_create(ui->screen_3);
    lv_label_set_text(ui->screen_3_label_status, "Ready");
    lv_obj_set_pos(ui->screen_3_label_status, 0, 60);
    lv_obj_set_width(ui->screen_3_label_status, 240);
    lv_obj_set_style_text_color(ui->screen_3_label_status, lv_color_hex(0xcccccc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_label_status, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_3_label_status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_label_status, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_label_status, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ========== 录音时长显示(大字体) ========== */
    ui->screen_3_label_time = lv_label_create(ui->screen_3);
    lv_label_set_text(ui->screen_3_label_time, "00:00");
    lv_obj_set_pos(ui->screen_3_label_time, 0, 110);
    lv_obj_set_width(ui->screen_3_label_time, 240);
    lv_obj_set_style_text_color(ui->screen_3_label_time, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_label_time, &lv_font_AlexBrush_Regular_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_3_label_time, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_label_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_label_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ========== 文件大小提示 ========== */
    ui->screen_3_label_size = lv_label_create(ui->screen_3);
    lv_label_set_text(ui->screen_3_label_size, "0 KB");
    lv_obj_set_pos(ui->screen_3_label_size, 0, 170);
    lv_obj_set_width(ui->screen_3_label_size, 240);
    lv_obj_set_style_text_color(ui->screen_3_label_size, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_label_size, &lv_font_montserratMedium_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_3_label_size, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_label_size, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_label_size, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ========== 录音/停止按钮(中间, 复用Play) ========== */
    ui->screen_3_btn_rec = lv_btn_create(ui->screen_3);
    lv_obj_set_pos(ui->screen_3_btn_rec, 88, 210);
    lv_obj_set_size(ui->screen_3_btn_rec, 64, 40);
    lv_obj_set_style_bg_color(ui->screen_3_btn_rec, lv_color_hex(0xff4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_btn_rec, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_3_btn_rec, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_3_btn_rec, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_btn_rec, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* 按下状态 */
    lv_obj_set_style_bg_color(ui->screen_3_btn_rec, lv_color_hex(0xcc3333), LV_PART_MAIN | LV_STATE_PRESSED);

    ui->screen_3_btn_rec_label = lv_label_create(ui->screen_3_btn_rec);
    lv_label_set_text(ui->screen_3_btn_rec_label, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(ui->screen_3_btn_rec_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_btn_rec_label, &lv_font_montserratMedium_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(ui->screen_3_btn_rec_label);

    /* ========== 暂停按钮(右侧, 复用Next) ========== */
    ui->screen_3_btn_pause = lv_btn_create(ui->screen_3);
    lv_obj_set_pos(ui->screen_3_btn_pause, 168, 210);
    lv_obj_set_size(ui->screen_3_btn_pause, 50, 40);
    lv_obj_set_style_bg_color(ui->screen_3_btn_pause, lv_color_hex(0x555577), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_btn_pause, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_3_btn_pause, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_3_btn_pause, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_btn_pause, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_3_btn_pause, lv_color_hex(0x444466), LV_PART_MAIN | LV_STATE_PRESSED);

    ui->screen_3_btn_pause_label = lv_label_create(ui->screen_3_btn_pause);
    lv_label_set_text(ui->screen_3_btn_pause_label, LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_color(ui->screen_3_btn_pause_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_btn_pause_label, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(ui->screen_3_btn_pause_label);

    /* ========== 返回按钮(左侧) ========== */
    ui->screen_3_btn_back = lv_btn_create(ui->screen_3);
    lv_obj_set_pos(ui->screen_3_btn_back, 22, 210);
    lv_obj_set_size(ui->screen_3_btn_back, 50, 40);
    lv_obj_set_style_bg_color(ui->screen_3_btn_back, lv_color_hex(0x555577), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_btn_back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_3_btn_back, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_3_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_3_btn_back, lv_color_hex(0x444466), LV_PART_MAIN | LV_STATE_PRESSED);

    ui->screen_3_btn_back_label = lv_label_create(ui->screen_3_btn_back);
    lv_label_set_text(ui->screen_3_btn_back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(ui->screen_3_btn_back_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_btn_back_label, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(ui->screen_3_btn_back_label);

    /* ========== 操作提示文字 ========== */
    ui->screen_3_label_hint = lv_label_create(ui->screen_3);
    lv_label_set_text(ui->screen_3_label_hint, "KEY2:REC  KEY1:PAUSE  WAKE:EXIT");
    lv_obj_set_pos(ui->screen_3_label_hint, 0, 270);
    lv_obj_set_width(ui->screen_3_label_hint, 240);
    lv_obj_set_style_text_color(ui->screen_3_label_hint, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_label_hint, &lv_font_montserratMedium_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_3_label_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_label_hint, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_label_hint, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ========== LED指示标签 ========== */
    ui->screen_3_label_led = lv_label_create(ui->screen_3);
    lv_label_set_text(ui->screen_3_label_led, LV_SYMBOL_STOP);
    lv_obj_set_pos(ui->screen_3_label_led, 110, 88);
    lv_obj_set_style_text_color(ui->screen_3_label_led, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_3_label_led, &lv_font_montserratMedium_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_3_label_led, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_3_label_led, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}
