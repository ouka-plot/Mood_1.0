/**
 ****************************************************************************************************
 * @file        tinyml_task.h
 * @brief       TinyML 推理 FreeRTOS 任务
 * @details     工作流: 等待通知 → 从环形缓冲取1秒音频 → 降噪 → MFCC → 推理 → 更新UI
 ****************************************************************************************************
 */

#ifndef __TINYML_TASK_H
#define __TINYML_TASK_H

#include "FreeRTOS.h"
#include "task.h"

/* 推理任务配置 */
#define TINYML_TASK_STACK_SIZE  768     /* 栈大小 (words), 3KB */
#define TINYML_TASK_PRIORITY    1       /* 低于音频/UI任务 */

/* 任务句柄 (main.c中定义) */
extern TaskHandle_t g_tinyml_task_handle;

/* 任务函数 */
void vTinyML_Task(void *pvParameters);

#endif /* __TINYML_TASK_H */
