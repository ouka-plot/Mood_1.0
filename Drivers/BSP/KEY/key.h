/**
 ****************************************************************************************************
 * @file        led.h
 * @author      ouka
 * @version     V1.0
 * @date        2025-11-12
 * @brief       KEY 按键输入 驱动代码
 *@notes        KEY0、KEY1 和 KEY2 采样到 低电平为有效
                KEY_UP 采样到高电平才为按键有效
                按键外部没有上下拉电阻，需要内部设置上下拉
                
                KEY0 – PE4
                KEY1 – PE3
                KEY2 – PE2
                KEY_WAKE–PA0
          *
 * 修改说明
 * V1.0
 * 第一次发布
 ****************************************************************************************************
 */


#ifndef __KEY_H_
#define __KEY_H_

#include "SYSTEM/SYS/sys.h"
/*----------------引脚定义 端口使能----------------------------*/

#define KEY0_PORT                GPIOE
#define KEY0_PIN                 GPIO_PIN_4
#define KEY0_GPIO_CLK_ENABLE()  do{__HAL_RCC_GPIOE_CLK_ENABLE();}while(0)


#define KEY1_PORT                GPIOE
#define KEY1_PIN                 GPIO_PIN_3
#define KEY1_GPIO_CLK_ENABLE()   do{__HAL_RCC_GPIOE_CLK_ENABLE();}while(0)


#define KEY2_PORT                GPIOE
#define KEY2_PIN                 GPIO_PIN_2
#define KEY2_GPIO_CLK_ENABLE()   do{__HAL_RCC_GPIOE_CLK_ENABLE();}while(0)


#define KEY_WAKE_PORT                 GPIOA
#define KEY_WAKE_PIN                  GPIO_PIN_0
#define KEY_WAKE_GPIO_CLK_ENABLE()   do{__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)

#define KEY0_STATE()               HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN)
#define KEY1_STATE()               HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN)
#define KEY2_STATE()               HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN)
#define KEY_WAKE_STATE()          HAL_GPIO_ReadPin(KEY_WAKE_PORT, KEY_WAKE_PIN)

// 使用位掩码(bitmask)来定义按键，支持组合键
// 每个按键占据一个bit
#define KEY0_PRES       (1 << 0)    // 值为 1
#define KEY1_PRES       (1 << 1)    // 值为 2
#define KEY2_PRES       (1 << 2)    // 值为 4
#define KEY_WAKE_PRES    (1 << 3)    // 值为 8


/* 注意：s_key_state 等变量已移到 key.c 中定义，避免多翻译单元重复实例 */
#define DEBOUNCE_TIME_MS    10     // 定义10ms的消抖时间（优化：机械按键抖动通常5-10ms）

void key_init(void);
void KEY_Tick(void);

uint8_t KEY_GetState(void);
uint8_t KEY_GetKeyDown(void);
uint8_t KEY_GetKeyUp(void);

#endif

