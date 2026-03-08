/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-14
 * @brief       template 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 * @notes 
 *
 ****************************************************************************************************
 */

#include "SYSTEM/sys/sys.h"
#include "SYSTEM/usart/usart.h"
#include "SYSTEM/delay/delay.h"
#include "BSP/LED/led.h"
#include "BSP/KEY/key.h"
#include "BSP/LCD/lcd.h"
#include "./USMART/usmart.h"
#include "./BSP/SDIO/sdio_sdcard.h"
  #include "./TEXT/text.h"
#include "./BSP/ES8388/es8388.h"
#include "./APP/audioplay.h"
#include "./APP/audio_monitor.h"
#include "./APP/tinyml_task.h"
#include "./APP/audio_spectrum.h"
#include "./APP/audio_eq.h"
#include "./APP/eq_ctrl_uart.h"

#include "./FATFS/source/ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "gui_guider.h"
#include "./APP/audio_ui_bridge.h"



//定义全局的文件系统对象
FATFS fs;
FIL file;
UINT br;
uint8_t read_buf[100];

/* FreeRTOS堆放到CCM RAM (0x10000000, 64KB)
 * 使用 NOLOAD 段避免64KB零数据占用FLASH
 * heap_4 通过 configAPPLICATION_ALLOCATED_HEAP 使用此外部数组 */
uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".ccmram_bss")));


/**
 * @brief       通过串口打印SD卡相关信息
 * @param       无
 * @retval      无
 */
void show_sdcard_info(void)
{
    HAL_SD_CardCIDTypeDef sd_card_cid;

    HAL_SD_GetCardCID(&g_sdcard_handler, &sd_card_cid); /* 获取CID */
    get_sd_card_info(&g_sd_card_info_handle);           /* 获取SD卡信息 */

    switch (g_sd_card_info_handle.CardType)
    {
    case CARD_SDSC:
    {
        if (g_sd_card_info_handle.CardVersion == CARD_V1_X)
        {
            printf("Card Type:SDSC V1\r\n");
        }
        else if (g_sd_card_info_handle.CardVersion == CARD_V2_X)
        {
            printf("Card Type:SDSC V2\r\n");
        }
    }
    break;

    case CARD_SDHC_SDXC:
        printf("Card Type:SDHC\r\n");
        break;
    default: break;
    }

    printf("Card ManufacturerID:%d\r\n", sd_card_cid.ManufacturerID);                   /* 制造商ID */
    printf("Card RCA:%d\r\n", g_sd_card_info_handle.RelCardAdd);                        /* 卡相对地址 */
    printf("LogBlockNbr:%d \r\n", (uint32_t)(g_sd_card_info_handle.LogBlockNbr));       /* 显示逻辑块数量 */
    printf("LogBlockSize:%d \r\n", (uint32_t)(g_sd_card_info_handle.LogBlockSize));     /* 显示逻辑块大小 */
    printf("Card Capacity:%d MB\r\n", (uint32_t)SD_TOTAL_SIZE_MB(&g_sdcard_handler));   /* 显示容量 */
    printf("Card BlockSize:%d\r\n\r\n", g_sd_card_info_handle.BlockSize);               /* 显示块大小 */
}



/* LED状态设置函数 */
void led_set(uint8_t sta)
{
    LED1(sta);
}

/* 函数参数调用测试函数 */
void test_fun(void(*ledset)(uint8_t), uint8_t sta)
{
    ledset(sta);
}


static void vLed_Task(void *pvParameters)
{
	static portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	for (;;)
	{
		LED1_TOGGLE();
		vTaskDelayUntil(&xLastWakeTime, 1000/portTICK_RATE_MS);
	}
}

/* LVGL GUI task - calls lv_timer_handler() periodically */
static void vLvgl_Task(void *pvParameters)
{
	for (;;)
	{
		audio_ui_update_lvgl();  /* 将音频状态同步到LVGL控件 */
		lv_timer_handler();
		vTaskDelay(5 / portTICK_RATE_MS);  /* 5ms period */
	}
}

static void vEqCtrl_Task(void *pvParameters)
{
	for (;;)
	{
		eq_ctrl_uart_poll();
		vTaskDelay(5 / portTICK_RATE_MS);
	}
}



static void vAudio_Task(void *pvParameters)
{
	audio_spectrum_init();  /* 初始化频谱分析模块 */
	audio_play();
	vTaskDelay(10);
	for (;;)
	{
	
	}
}

/* 音频检测回调函数 - 检测到高频声音(婴儿哭声)时调用 */
static void audio_detect_callback(uint8_t detected, uint32_t energy, uint16_t zcr)
{
    if (detected)
    {
        printf("[AudioMonitor] High-freq sound DETECTED! Energy=%lu, ZCR=%u\r\n", energy, zcr);
        LED0(0);  /* 点亮LED0表示检测到 */
    }
    else
    {
        printf("[AudioMonitor] Sound stopped. Energy=%lu, ZCR=%u\r\n", energy, zcr);
        LED0(1);  /* 熄灭LED0 */
    }
}

/* 音频监听任务外部声明 */
extern TaskHandle_t g_audio_monitor_task_handle;


int main(void)
{ 


	//必须选择为组4，中断均为抢占优先级
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	//屏蔽中断，防止系统没有启动，而在中断里调用了系统的函数
	__set_PRIMASK(1);


/*---------------------------初始化-------------------------*/
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
	usart_init(115200);                     /* 串口初始化为115200 */
 	usmart_init(84);                        /* USMART初始化 */
    led_init();                         /* 初始化LED */
    key_init();                         /* 初始化KEY */
	lcd_init();  
                               /* 初始化LCD */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* 初始化播放器界面(screen_1)并加载 */
    audio_ui_init_screen();

	LED0(1);
	
	//      while (sd_init())                       /* 检测SD卡 */
    // {
    //     lcd_show_string(30, 50, 200, 16, 16, "SD Card Failed!", RED);
    //     delay_ms(200);
    //     lcd_fill(30, 50, 200 + 30, 50 + 16, WHITE);
    //     delay_ms(200);
    // }
    f_mount(&fs, "0:", 1);        /* 挂载SD卡 */

    
    es8388_init();              /* ES8388初始化 */
    es8388_adda_cfg(1, 0);      /* 开启DAC关闭ADC */
    es8388_output_cfg(1, 0);    /* OUT1(耳机)开, OUT2(喇叭)关 */
    es8388_hpvol_set(10);       /* 设置耳机音量 (0~33) */
    es8388_spkvol_set(0);       /* 设置喇叭音量 */
	audio_ui_set_volume(33);     /* 同步默认耳机音量到UI(10/30约等于33%) */
	audio_eq_init();
	eq_ctrl_uart_init(115200);
    
    // text_show_string(30, 30, 200, 16, "正点原子STM32开发板", 16, 0, RED);
    // text_show_string(30, 50, 200, 16, "音乐播放器实验", 16, 0, RED);
    // text_show_string(30, 70, 200, 16, "正点原子@ALIENTEK", 16, 0, RED);
    // text_show_string(30, 90, 200, 16, "2021年11月16日", 16, 0, RED);
    // text_show_string(30, 110, 200, 16, "KEY0:NEXT   KEY2:PREV", 16, 0, RED);
    // text_show_string(30, 130, 200, 16, "KEY_UP:PAUSE/PLAY", 16, 0, RED);
    
	
		xTaskCreate(vAudio_Task, \
				"Audio", \
				1024, \
				NULL, \
				3, \
				NULL);
	xTaskCreate(vLed_Task, \
				"Led", \
				configMINIMAL_STACK_SIZE, \
				NULL, \
				2, \
				NULL);

	xTaskCreate(vLvgl_Task, \
				"LVGL", \
				512, \
				NULL, \
				2, \
				NULL);

	xTaskCreate(vEqCtrl_Task, \
				"EqCtrl", \
				256, \
				NULL, \
				1, \
				NULL);
	
	/* 创建音频监听任务 */
	xTaskCreate(vAudioMonitor_Task, \
				"AudioMon", \
				256, \
				NULL, \
				2, \
				&g_audio_monitor_task_handle);
	
	/* 设置检测回调并启动后台监听 */
	audio_monitor_set_callback(audio_detect_callback);
	/* 注意：实际启动监听需要在音频系统初始化好后调用 audio_monitor_start() */
	
	/* 创建TinyML推理任务 */
	xTaskCreate(vTinyML_Task, \
				"TinyML", \
				TINYML_TASK_STACK_SIZE, \
				NULL, \
				TINYML_TASK_PRIORITY, \
				&g_tinyml_task_handle);
	
	vTaskStartScheduler();
	
	
	
    while (1)
    {
        //audio_play();           /* 播放音乐 */
    }
 }

 
//----------------------------------------------------------------------
//FreeRTOS 钩子函数
void vApplicationMallocFailedHook(void)
{
	printf("[FreeRTOS] ERROR: pvPortMalloc failed!\r\n");
	for (;;)
	{
		vTaskDelay(portMAX_DELAY);
	}
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	printf("[FreeRTOS] ERROR: Stack overflow! Task: %s\r\n", pcTaskName);
	for (;;)
	{
		vTaskDelay(portMAX_DELAY);
	}
}
