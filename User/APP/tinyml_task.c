/**
 ****************************************************************************************************
 * @file        tinyml_task.c
 * @brief       TinyML 推理 FreeRTOS 任务实现
 * @details     低优先级后台任务, 每秒运行一次推理流水线:
 *
 *              1. 等待 audio_monitor 通知 (1秒音频积累完成)
 *              2. 从环形缓冲拍快照 → 16000个int16采样
 *              3. 分帧(512样本/帧, 31帧), 每帧:
 *                 a) 谱减法降噪
 *                 b) MFCC特征提取 → 13个系数
 *              4. 组成特征矩阵 [31 × 13]
 *              5. CNN前向传播 → 5类概率
 *              6. 更新 audio_ui_bridge 中的分类结果
 *              7. 串口输出调试信息
 *
 *              内存: 1秒快照缓冲 32KB + MFCC矩阵 1.6KB, 从CCM heap
 ****************************************************************************************************
 */

#include "tinyml_task.h"
#include "audio_ringbuf.h"
#include "audio_denoise.h"
#include "audio_features.h"
#include "audio_classify.h"
#include "audio_ui_bridge.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* 任务句柄 */
TaskHandle_t g_tinyml_task_handle = NULL;

/* 工作缓冲 (从CCM heap分配) */
static int16_t    s_frame_tmp[MFCC_FFT_LEN];   /* 栈上帧缓冲 512×2=1KB (静态避免栈溢出) */
static float32_t  *s_mfcc_matrix = NULL;        /* [15 × 13] = 780B */

void vTinyML_Task(void *pvParameters)
{
    (void)pvParameters;

    /* ===== 初始化所有子模块 ===== */
    int ret;

    ret = audio_ringbuf_init();
    if (ret != 0) {
        printf("[TinyML] ERROR: ringbuf init failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    ret = audio_denoise_init();
    if (ret != 0) {
        printf("[TinyML] ERROR: denoise init failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    ret = audio_features_init();
    if (ret != 0) {
        printf("[TinyML] ERROR: features init failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    ret = audio_classify_init();
    if (ret != 0) {
        printf("[TinyML] ERROR: classify init failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    /* 分配MFCC矩阵 (仅780B) */
    s_mfcc_matrix = pvPortMalloc(MFCC_FRAMES_PER_SEC * MFCC_NUM_COEFFS * sizeof(float32_t));

    if (!s_mfcc_matrix) {
        printf("[TinyML] ERROR: buffer alloc failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    printf("[TinyML] Initialized OK. Waiting for audio...\r\n");

    /* ===== 主循环 ===== */
    for (;;)
    {
        /* 等待 audio_monitor 的通知 (音频就绪) */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t t_start = xTaskGetTickCount();

        /* 逐帧从ringbuf读取 → 降噪 → MFCC, 无需32KB快照 */
        for (uint32_t f = 0; f < MFCC_FRAMES_PER_SEC; f++)
        {
            /* 从ringbuf直接读帧到临时缓冲(1KB) */
            audio_ringbuf_read_frame(f, MFCC_FFT_LEN, s_frame_tmp);

            /* 谱减法降噪 (原地处理) */
            audio_denoise_process(s_frame_tmp);

            /* MFCC特征提取 → 写入特征矩阵的第f行 */
            audio_features_compute_frame(s_frame_tmp,
                                          &s_mfcc_matrix[f * MFCC_NUM_COEFFS]);
        }

        /* 重置降噪的噪声估计 */
        audio_denoise_reset();

        /* CNN推理 */
        ClassifyResult_t result;
        ret = audio_classify_run(s_mfcc_matrix, &result);

        TickType_t t_end = xTaskGetTickCount();
        uint32_t elapsed_ms = (t_end - t_start) * portTICK_RATE_MS;

        if (ret == 0)
        {
            /* 更新 audio_ui_bridge */
            audio_ui_set_classify_result(result.label, result.confidence);

            /* 串口调试输出 (newlib-nano不支持%f, 用整数代替) */
            uint32_t conf_pct = (uint32_t)(result.confidence * 100.0f);
            uint32_t conf_dec = (uint32_t)(result.confidence * 1000.0f) % 10;
            printf("[TinyML] %s (%lu.%lu%%) [%lums]\r\n",
                   g_class_names[result.label],
                   conf_pct, conf_dec,
                   elapsed_ms);
        }
    }
}
