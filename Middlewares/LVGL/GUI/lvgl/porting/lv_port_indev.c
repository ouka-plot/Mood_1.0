/**
 * @file lv_port_indev.c
 * @brief LVGL input device driver - Keypad only (minimal for Mood_1.0)
 */

#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"
#include "key.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_keypad;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    /* Initialize keypad hardware */
    keypad_init();

    /* Register keypad input device */
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv);

    /* Create a default group and assign keypad to it */
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(indev_keypad, g);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void keypad_init(void)
{
    /* key_init() already called in main.c */
}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    /* 按键由音频任务(wavplay.c)的KEY_Tick()+KEY_GetKeyDown()处理
     * LVGL键盘输入不消费按键事件，避免竞争
     * 未来实现屏幕切换时可在此添加KEY_WAKE处理 */
    data->state = LV_INDEV_STATE_RELEASED;
    data->key = 0;
}

#else
typedef int keep_pedantic_happy;
#endif
