/**
 ****************************************************************************************************
 * @file        audio_classify.h
 * @brief       音频分类推理模块 - 轻量CNN前向传播引擎
 * @details     固定架构: Conv1D(16)→ReLU→MaxPool→Conv1D(32)→ReLU→GAvgPool→Dense(5)→Softmax
 *              输入: 31帧×13 MFCC 特征矩阵 (float32)
 *              输出: 5类概率 (silence, noise, baby_cry, shout, speech)
 ****************************************************************************************************
 */

#ifndef __AUDIO_CLASSIFY_H
#define __AUDIO_CLASSIFY_H

#include "sys.h"
#include "arm_math.h"
#include "audio_features.h"

/* ======================== 分类类别 ======================== */
#define CLASSIFY_NUM_CLASSES     5

typedef enum {
    CLASS_SILENCE   = 0,
    CLASS_NOISE     = 1,
    CLASS_BABY_CRY  = 2,
    CLASS_SHOUT     = 3,
    CLASS_SPEECH    = 4
} AudioClass_t;

/* 类别名称 (用于调试/UI显示) */
extern const char * const g_class_names[CLASSIFY_NUM_CLASSES];

/* ======================== 模型参数 ======================== */
/* Conv1D 层1: input_ch=13, filters=16, kernel=3 */
#define CONV1_IN_CH     13
#define CONV1_FILTERS   16
#define CONV1_KERNEL    3

/* MaxPool1D: pool=2 */
#define POOL1_SIZE      2

/* Conv1D 层2: input_ch=16, filters=32, kernel=3 */
#define CONV2_IN_CH     16
#define CONV2_FILTERS   32
#define CONV2_KERNEL    3

/* Dense 输出层: input=32, output=5 */
#define DENSE_IN        32
#define DENSE_OUT       CLASSIFY_NUM_CLASSES

/* 中间维度 (从输入15帧推算) */
#define INPUT_FRAMES    MFCC_FRAMES_PER_SEC     /* 帧数 (由audio_features.h定义) */
#define INPUT_FEATURES  13      /* MFCC系数个数 */
#define AFTER_CONV1     (INPUT_FRAMES - CONV1_KERNEL + 1)   /* 13 (15帧时) */
#define AFTER_POOL1     (AFTER_CONV1 / POOL1_SIZE)           /* 6 */
#define AFTER_CONV2     (AFTER_POOL1 - CONV2_KERNEL + 1)     /* 4 */

/* ======================== 推理结果 ======================== */
typedef struct {
    AudioClass_t    label;                          /* 预测类别 */
    float32_t       confidence;                     /* 最高类别的概率 */
    float32_t       probs[CLASSIFY_NUM_CLASSES];    /* 各类概率 */
} ClassifyResult_t;

/* ======================== 函数接口 ======================== */

/**
 * @brief  初始化分类推理模块
 * @note   从CCM heap分配中间缓冲区 (~6KB)
 *         必须在FreeRTOS调度器启动后调用
 * @retval 0=成功, -1=内存分配失败, -2=模型未就绪
 */
int audio_classify_init(void);

/**
 * @brief  运行一次推理
 * @param  mfcc_input: MFCC特征矩阵 [INPUT_FRAMES × INPUT_FEATURES] (float32, 行优先)
 * @param  result    : 输出分类结果
 * @retval 0=成功, -1=未初始化
 */
int audio_classify_run(const float32_t *mfcc_input, ClassifyResult_t *result);

/**
 * @brief  获取模块是否已初始化
 * @retval 1=已初始化, 0=未初始化
 */
uint8_t audio_classify_is_ready(void);

#endif /* __AUDIO_CLASSIFY_H */
