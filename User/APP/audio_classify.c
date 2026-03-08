/**
 ****************************************************************************************************
 * @file        audio_classify.c
 * @brief       音频分类推理模块 - 轻量CNN前向传播引擎
 * @details     实现固定架构CNN的逐层前向传播:
 *              1. Conv1D(16, k=3) + ReLU:  [31,13] → [29,16]
 *              2. MaxPool1D(2):             [29,16] → [14,16]
 *              3. Conv1D(32, k=3) + ReLU:  [14,16] → [12,32]
 *              4. GlobalAvgPool1D:          [12,32] → [32]
 *              5. Dense(5) + Softmax:       [32]    → [5]
 *
 *              所有中间缓冲区从CCM heap分配, 权重在Flash(const)。
 *              使用CMSIS-DSP arm_dot_prod_f32 加速向量点积。
 ****************************************************************************************************
 */

#include "audio_classify.h"
#include "model_data.h"
#include "FreeRTOS.h"
#include <math.h>
#include <string.h>

/* ======================== 类别名称 ======================== */
const char * const g_class_names[CLASSIFY_NUM_CLASSES] = {
    "silence",
    "noise",
    "baby_cry",
    "shout",
    "speech"
};

/* ======================== 私有变量 ======================== */
static uint8_t s_initialized = 0;

/* 中间缓冲区 (从CCM heap分配) */
static float32_t *s_buf_a = NULL;   /* 较大缓冲: max(29*16, 14*16, 12*32) = 464 */
static float32_t *s_buf_b = NULL;   /* 较大缓冲: 同上 */
static float32_t *s_vec   = NULL;   /* 向量缓冲: max(32, 5) */

#define BUF_A_SIZE  (AFTER_CONV1 * CONV1_FILTERS)   /* 29*16 = 464 */
#define BUF_B_SIZE  (AFTER_CONV2 * CONV2_FILTERS)   /* 12*32 = 384 */
#define VEC_SIZE    DENSE_IN                          /* 32 */

/* ======================== 层实现 ======================== */

/**
 * @brief  Conv1D + ReLU 前向传播
 * @param  in       : 输入 [frames × in_ch], 行优先
 * @param  out      : 输出 [out_frames × filters], 行优先
 * @param  weights  : 卷积核 [kernel × in_ch × filters]
 * @param  bias     : 偏置 [filters]
 * @param  frames   : 输入时间帧数
 * @param  in_ch    : 输入通道数
 * @param  filters  : 输出滤波器数
 * @param  kernel   : 卷积核大小
 */
static void conv1d_relu(const float32_t *in, float32_t *out,
                         const float32_t *weights, const float32_t *bias,
                         uint32_t frames, uint32_t in_ch,
                         uint32_t filters, uint32_t kernel)
{
    uint32_t out_frames = frames - kernel + 1;

    for (uint32_t t = 0; t < out_frames; t++) {
        for (uint32_t f = 0; f < filters; f++) {
            float32_t sum = bias[f];

            /* 对 kernel 个时间步 × in_ch 个通道做点积 */
            for (uint32_t k = 0; k < kernel; k++) {
                const float32_t *in_ptr = &in[(t + k) * in_ch];
                const float32_t *w_ptr  = &weights[(k * in_ch + 0) * filters + f];

                /* 逐通道累加: w[k][c][f] * in[t+k][c] */
                for (uint32_t c = 0; c < in_ch; c++) {
                    sum += in_ptr[c] * weights[(k * in_ch + c) * filters + f];
                }
            }

            /* ReLU */
            out[t * filters + f] = (sum > 0.0f) ? sum : 0.0f;
        }
    }
}

/**
 * @brief  MaxPool1D 前向传播
 * @param  in       : 输入 [frames × ch]
 * @param  out      : 输出 [frames/pool × ch]
 * @param  frames   : 输入帧数
 * @param  ch       : 通道数
 * @param  pool     : 池化窗口大小
 */
static void maxpool1d(const float32_t *in, float32_t *out,
                       uint32_t frames, uint32_t ch, uint32_t pool)
{
    uint32_t out_frames = frames / pool;

    for (uint32_t t = 0; t < out_frames; t++) {
        for (uint32_t c = 0; c < ch; c++) {
            float32_t max_val = in[(t * pool) * ch + c];
            for (uint32_t p = 1; p < pool; p++) {
                float32_t val = in[(t * pool + p) * ch + c];
                if (val > max_val) max_val = val;
            }
            out[t * ch + c] = max_val;
        }
    }
}

/**
 * @brief  GlobalAveragePooling1D
 * @param  in       : 输入 [frames × ch]
 * @param  out      : 输出 [ch]
 * @param  frames   : 时间帧数
 * @param  ch       : 通道数
 */
static void global_avg_pool(const float32_t *in, float32_t *out,
                             uint32_t frames, uint32_t ch)
{
    float32_t inv_frames = 1.0f / (float32_t)frames;

    for (uint32_t c = 0; c < ch; c++) {
        float32_t sum = 0.0f;
        for (uint32_t t = 0; t < frames; t++) {
            sum += in[t * ch + c];
        }
        out[c] = sum * inv_frames;
    }
}

/**
 * @brief  Dense (全连接层) + Softmax
 * @param  in       : 输入 [in_dim]
 * @param  out      : 输出概率 [out_dim]
 * @param  weights  : 权重 [in_dim × out_dim], 行优先
 * @param  bias     : 偏置 [out_dim]
 * @param  in_dim   : 输入维度
 * @param  out_dim  : 输出维度
 */
static void dense_softmax(const float32_t *in, float32_t *out,
                           const float32_t *weights, const float32_t *bias,
                           uint32_t in_dim, uint32_t out_dim)
{
    /* 线性变换: out = in * W + b */
    float32_t max_logit = -1e30f;

    for (uint32_t j = 0; j < out_dim; j++) {
        float32_t sum = bias[j];
        for (uint32_t i = 0; i < in_dim; i++) {
            sum += in[i] * weights[i * out_dim + j];
        }
        out[j] = sum;
        if (sum > max_logit) max_logit = sum;
    }

    /* Softmax (数值稳定版: 减去max防止exp溢出) */
    float32_t exp_sum = 0.0f;
    for (uint32_t j = 0; j < out_dim; j++) {
        out[j] = expf(out[j] - max_logit);
        exp_sum += out[j];
    }
    float32_t inv_sum = 1.0f / exp_sum;
    for (uint32_t j = 0; j < out_dim; j++) {
        out[j] *= inv_sum;
    }
}

/* ======================== 公有接口 ======================== */

int audio_classify_init(void)
{
    if (s_initialized) return 0;

    /* 从CCM heap分配中间缓冲 */
    s_buf_a = pvPortMalloc(BUF_A_SIZE * sizeof(float32_t));   /* 1856 B */
    s_buf_b = pvPortMalloc(BUF_B_SIZE * sizeof(float32_t));   /* 1536 B */
    s_vec   = pvPortMalloc(VEC_SIZE * sizeof(float32_t));      /* 128 B  */

    if (!s_buf_a || !s_buf_b || !s_vec) {
        return -1;
    }

#if !MODEL_TRAINED
    /* 权重全零 — 推理结果无意义, 但流程完整可测试 */
#endif

    s_initialized = 1;
    return 0;
}

int audio_classify_run(const float32_t *mfcc_input, ClassifyResult_t *result)
{
    if (!s_initialized || !mfcc_input || !result) return -1;

    /* Layer 1: Conv1D(16, k=3) + ReLU
     * in: mfcc_input [31 × 13]  → out: s_buf_a [29 × 16] */
    conv1d_relu(mfcc_input, s_buf_a,
                conv1_weights, conv1_bias,
                INPUT_FRAMES, CONV1_IN_CH, CONV1_FILTERS, CONV1_KERNEL);

    /* Layer 2: MaxPool1D(2)
     * in: s_buf_a [29 × 16] → out: s_buf_b [14 × 16]
     * 注: 29/2=14 (向下取整, 最后1帧丢弃) */
    maxpool1d(s_buf_a, s_buf_b,
              AFTER_CONV1, CONV1_FILTERS, POOL1_SIZE);

    /* Layer 3: Conv1D(32, k=3) + ReLU
     * in: s_buf_b [14 × 16] → out: s_buf_a [12 × 32]
     * 复用 s_buf_a (足够大: 464 >= 384) */
    conv1d_relu(s_buf_b, s_buf_a,
                conv2_weights, conv2_bias,
                AFTER_POOL1, CONV2_IN_CH, CONV2_FILTERS, CONV2_KERNEL);

    /* Layer 4: GlobalAveragePool1D
     * in: s_buf_a [12 × 32] → out: s_vec [32] */
    global_avg_pool(s_buf_a, s_vec, AFTER_CONV2, CONV2_FILTERS);

    /* Layer 5: Dense(5) + Softmax
     * in: s_vec [32] → out: result->probs [5] */
    dense_softmax(s_vec, result->probs,
                  dense_weights, dense_bias,
                  DENSE_IN, DENSE_OUT);

    /* 找最大概率类别 */
    float32_t max_prob = result->probs[0];
    uint32_t max_idx = 0;
    for (uint32_t i = 1; i < CLASSIFY_NUM_CLASSES; i++) {
        if (result->probs[i] > max_prob) {
            max_prob = result->probs[i];
            max_idx = i;
        }
    }
    result->label      = (AudioClass_t)max_idx;
    result->confidence = max_prob;

    return 0;
}

uint8_t audio_classify_is_ready(void)
{
    return s_initialized;
}
