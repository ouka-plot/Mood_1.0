#include "BSP/KEY/key.h"
#include "SYSTEM/delay/delay.h"

/* 按键状态变量（全局唯一实例） */
static uint8_t s_key_state;         // 按键的当前持续状态
static uint8_t s_key_down;          // 按键的"按下"事件
static uint8_t s_key_up;            // 按键的"松开"事件
static uint8_t s_debounce_state;    // 消抖过程中的中间状态
static uint8_t s_debounce_counter;  // 消抖计数器



void key_init()
{
    GPIO_InitTypeDef gpio_init_struct;              
    KEY0_GPIO_CLK_ENABLE();                            /* 时钟使能 */
    KEY1_GPIO_CLK_ENABLE();                             
    KEY2_GPIO_CLK_ENABLE();                             

    gpio_init_struct.Pin = KEY0_PIN;                   
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                /* 输入 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(KEY0_PORT, &gpio_init_struct);       /* 初始化引脚 */

    gpio_init_struct.Pin = KEY1_PIN;                   
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                /* 输入 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(KEY1_PORT, &gpio_init_struct);       /* 初始化引脚 */

    gpio_init_struct.Pin = KEY2_PIN;                   
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                /* 输入 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(KEY2_PORT, &gpio_init_struct);       /* 初始化引脚 */
	
	gpio_init_struct.Pin = KEY_WAKE_PIN;                   
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                /* 输入 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                    /* 下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(KEY_WAKE_PORT, &gpio_init_struct);       /* 初始化引脚 */

    s_key_state = 0;
    s_key_down = 0;
    s_key_up = 0;
    s_debounce_state = 0;
    s_debounce_counter = 0;
}


uint8_t KEY_GetState(void)
{
    return s_key_state;
}

uint8_t KEY_GetKeyDown(void)
{
    uint8_t temp_down = s_key_down;
    s_key_down = 0; // 读取后立即清除，确保事件只被响应一次
    return temp_down;
}

uint8_t KEY_GetKeyUp(void)
{
    uint8_t temp_up = s_key_up;
    s_key_up = 0;   // 读取后立即清除
    return temp_up;
}

// 这是整个驱动的灵魂
void KEY_Tick(void)     //循环调用
{
    uint8_t raw_key_state = 0;
    uint8_t old_key_state = s_key_state;

    // 1. 读取所有按键的原始状态，并组合成一个位掩码
    if (!KEY0_STATE()) 
		raw_key_state |= KEY0_PRES;
    if (!KEY1_STATE()) 
		raw_key_state |= KEY1_PRES;
    if (!KEY2_STATE()) 
		raw_key_state |= KEY2_PRES;
    if (KEY_WAKE_STATE()) 
		raw_key_state |= KEY_WAKE_PRES;

    // 2. 非阻塞式软件消抖
    if (raw_key_state != s_debounce_state)
    {
        // 状态发生变化，重置计数器
        s_debounce_state = raw_key_state;
        s_debounce_counter = 0;
    }
    else
    {
        // 状态保持稳定
        if (s_debounce_counter < (DEBOUNCE_TIME_MS / 5)) // 假设KEY_Tick每5ms调用一次
        {
            s_debounce_counter++;
        }
        else
        {
            // 稳定时间超过阈值，确认状态更新
            s_key_state = s_debounce_state;
        }
    }

    // 3. 计算"按下"和"松开"事件
    s_key_down = s_key_state & (old_key_state ^ s_key_state);
    s_key_up = ~s_key_state & (old_key_state ^ s_key_state);
}
