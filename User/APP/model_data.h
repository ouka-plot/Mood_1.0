/**
 ****************************************************************************************************
 * @file        model_data.h
 * @brief       CNN模型权重数据 - 占位文件
 * @details     训练完成后，用Python脚本生成的实际权重替换此文件中的零值数组。
 *              模型架构: Conv1D(16,k=3) → ReLU → MaxPool(2) → Conv1D(32,k=3) → ReLU
 *                        → GlobalAvgPool → Dense(5) → Softmax
 *
 *  权重生成方法 (Python):
 *    import numpy as np
 *    model = keras.models.load_model("audio_cnn.h5")
 *    for layer in model.layers:
 *        w = layer.get_weights()
 *        if w: print_c_array(w)
 ****************************************************************************************************
 */

#ifndef __MODEL_DATA_H
#define __MODEL_DATA_H

#include "arm_math.h"

/* 标记模型是否已训练 (0=占位零权重, 1=真实权重) */
#define MODEL_TRAINED   0

/*
 * Conv1D 层1: kernel=[3, 13, 16]  (kernel_size × in_ch × filters)
 * 权重存储: [kernel_size][in_ch][filters] 行优先 = 3×13×16 = 624 个float
 * 偏置: [16]
 */
#define CONV1_WEIGHT_SIZE   (3 * 13 * 16)   /* 624 */
#define CONV1_BIAS_SIZE     16

static const float32_t conv1_weights[CONV1_WEIGHT_SIZE] = { 0 };
static const float32_t conv1_bias[CONV1_BIAS_SIZE] = { 0 };

/*
 * Conv1D 层2: kernel=[3, 16, 32]
 * 权重: 3×16×32 = 1536 个float
 * 偏置: [32]
 */
#define CONV2_WEIGHT_SIZE   (3 * 16 * 32)   /* 1536 */
#define CONV2_BIAS_SIZE     32

static const float32_t conv2_weights[CONV2_WEIGHT_SIZE] = { 0 };
static const float32_t conv2_bias[CONV2_BIAS_SIZE] = { 0 };

/*
 * Dense 层: [32, 5]
 * 权重: 32×5 = 160 个float
 * 偏置: [5]
 */
#define DENSE_WEIGHT_SIZE   (32 * 5)    /* 160 */
#define DENSE_BIAS_SIZE     5

static const float32_t dense_weights[DENSE_WEIGHT_SIZE] = { 0 };
static const float32_t dense_bias[DENSE_BIAS_SIZE] = { 0 };

/* 总权重参数量: 624+16+1536+32+160+5 = 2373 floats = 9.3 KB */

#endif /* __MODEL_DATA_H */
