/**
 ****************************************************************************************************
 * @file        audio_ringbuf.c
 * @brief       音频环形缓冲区实现
 * @details     使用写指针 + 总计数器的方式实现无锁单生产者单消费者环形缓冲。
 *              写端: DMA回调(ISR) → audio_monitor → audio_ringbuf_write()
 *              读端: 推理任务 → audio_ringbuf_snapshot() 拷贝最近1秒
 *
 *              内存布局:
 *              s_ring_buf[0 ... RINGBUF_SIZE-1]  (32KB, 从CCM heap分配)
 *              写指针 s_write_idx 在 [0, RINGBUF_SIZE) 循环
 ****************************************************************************************************
 */

#include "audio_ringbuf.h"
#include "FreeRTOS.h"
#include <string.h>

/* ======================== 私有变量 ======================== */
static int16_t    *s_ring_buf   = NULL;   /* 环形缓冲区, CCM heap分配 */
static volatile uint32_t s_write_idx    = 0;  /* 下一个写入位置 [0, RINGBUF_SIZE) */
static volatile uint32_t s_total_written = 0; /* 累积写入采样数 (不回绕) */

/* ======================== 公有接口 ======================== */

int audio_ringbuf_init(void)
{
    if (s_ring_buf != NULL) return 0;  /* 已初始化 */

    s_ring_buf = pvPortMalloc(RINGBUF_SIZE * sizeof(int16_t));
    if (s_ring_buf == NULL) return -1;

    memset(s_ring_buf, 0, RINGBUF_SIZE * sizeof(int16_t));
    s_write_idx     = 0;
    s_total_written = 0;
    return 0;
}

void audio_ringbuf_write(const int16_t *data, uint32_t samples)
{
    if (s_ring_buf == NULL || samples == 0) return;

    uint32_t idx = s_write_idx;

    /* 分两段拷贝处理回绕 */
    uint32_t space_to_end = RINGBUF_SIZE - idx;

    if (samples <= space_to_end) {
        /* 一次拷贝即可 */
        memcpy(&s_ring_buf[idx], data, samples * sizeof(int16_t));
        idx += samples;
        if (idx >= RINGBUF_SIZE) idx = 0;
    } else {
        /* 先填满尾部, 再从头部继续 */
        memcpy(&s_ring_buf[idx], data, space_to_end * sizeof(int16_t));
        uint32_t remain = samples - space_to_end;
        memcpy(&s_ring_buf[0], &data[space_to_end], remain * sizeof(int16_t));
        idx = remain;
    }

    s_write_idx = idx;
    s_total_written += samples;
}

void audio_ringbuf_snapshot(int16_t *out)
{
    if (s_ring_buf == NULL || out == NULL) return;

    /*
     * 读取最近 RINGBUF_SIZE 个采样, 按时间顺序输出。
     * 读时暂存写指针(单次读取, 写端在ISR可能并发, 但32位读是原子的)。
     * 最坏情况: 快照期间写端更新了一帧(512样本), 导致极少量不连续,
     * 对MFCC特征影响可忽略。
     */
    uint32_t widx = s_write_idx;  /* 原子读 */

    /* widx 指向下一个写入位置, 即最老数据的位置 */
    uint32_t start = widx;  /* 最老的采样在 s_ring_buf[start] */

    uint32_t tail_len = RINGBUF_SIZE - start;

    if (tail_len >= RINGBUF_SIZE) {
        /* start == 0, 直接拷贝整个缓冲 */
        memcpy(out, s_ring_buf, RINGBUF_SIZE * sizeof(int16_t));
    } else {
        /* 先拷贝 [start..end), 再拷贝 [0..start) */
        memcpy(out, &s_ring_buf[start], tail_len * sizeof(int16_t));
        memcpy(&out[tail_len], &s_ring_buf[0], start * sizeof(int16_t));
    }
}

uint32_t audio_ringbuf_get_total_written(void)
{
    return s_total_written;
}

uint8_t audio_ringbuf_is_ready(void)
{
    return (s_total_written >= RINGBUF_SIZE) ? 1 : 0;
}

void audio_ringbuf_read_frame(uint32_t frame_idx, uint32_t frame_size, int16_t *out)
{
    if (s_ring_buf == NULL || out == NULL) return;

    uint32_t widx = s_write_idx;
    /* 最老数据起始位置 = widx (写指针指向下一个要覆盖的位置) */
    uint32_t start = (widx + frame_idx * frame_size) % RINGBUF_SIZE;

    uint32_t tail = RINGBUF_SIZE - start;
    if (frame_size <= tail) {
        memcpy(out, &s_ring_buf[start], frame_size * sizeof(int16_t));
    } else {
        memcpy(out, &s_ring_buf[start], tail * sizeof(int16_t));
        memcpy(&out[tail], &s_ring_buf[0], (frame_size - tail) * sizeof(int16_t));
    }
}

void audio_ringbuf_reset(void)
{
    s_write_idx     = 0;
    s_total_written = 0;

    if (s_ring_buf != NULL) {
        memset(s_ring_buf, 0, RINGBUF_SIZE * sizeof(int16_t));
    }
}
