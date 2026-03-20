#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>  //广播/连接  GAP层
#include <zephyr/bluetooth/conn.h>    //连接管理
#include <zephyr/bluetooth/gatt.h>  //GATT服务
#include <zephyr/bluetooth/hci.h>   //HCI底层协议
#include <bluetooth/services/nus.h>  //nordic uart service
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(mood_eq_bridge, LOG_LEVEL_INF);

#define DEVICE_NAME                 CONFIG_BT_DEVICE_NAME
#define BLE_CHUNK_SIZE              20U //每包最大数 20字节
#define RING_BUF_SIZE               512U //

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static struct bt_conn *current_conn; //ble 连接 句柄

/* ---------- Ring buffer: UART RX -> BLE TX ---------- */
static uint8_t ring_buf[RING_BUF_SIZE];
static volatile uint16_t ring_head;   /* ISR writes here  */
static volatile uint16_t ring_tail;   /* main loop reads  */

static uint16_t ring_count(void)
{
    uint16_t h = ring_head;
    uint16_t t = ring_tail;
    return (h >= t) ? (h - t) : (RING_BUF_SIZE - t + h);
}

/* ---------- BLE advertising ---------- */
//广播包 max 31B 由多个ad structure 组成

//通用可发现模式（可被所有人扫描到）

//仅 BLE，不支持经典蓝牙（BR/EDR）

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, sizeof(DEVICE_NAME) - 1),
};
//扫描响应包 SD 31B  128bit UUID
static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/* ---------- BLE send with retry ---------- */
static void ble_send_chunk(const uint8_t *data, uint16_t len)
{
    int err;
    int retries = 5;

    if ((current_conn == NULL) || (len == 0U))
    {
        return;
    }

    while (retries-- > 0)
    {
        

//1. 调用 GATT Notification 机制
//2. 把数据写入 TX Characteristic
//3. 通过 ATT_HANDLE_VALUE_NTF 发给已订阅的手机
        err = bt_nus_send(current_conn, data, len);
        if (err == 0) //发送成功
        {
            return;
        }
        if (err == -EAGAIN)
        {
            k_msleep(10);
//BLE 协议栈有 ACL 发送缓冲区限制：

//当协议栈内部发送队列满了
//返回 `EAGAIN`（等一会儿再试）
//这里最多重试 5 次，每次等 10ms，总超时约 50ms。
            continue; //ble 忙,等10ms重试
        }
        if ((err != -ENOTCONN))
        {
            LOG_WRN("bt_nus_send failed: %d (len=%u)", err, len);
        }
        return; //其他错误退出
    }
}

/* ---------- UART ISR: bytes -> ring buffer ---------- */
static void uart_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);
//nRF UARTE 驱动要求每次进 ISR 先刷新中断状态寄存器
//否则后续的 uart_irq_rx_ready() 返回值不可靠。
    if (!uart_irq_update(dev)) //更新中断状态
    {
        return;
    }

    while (uart_irq_rx_ready(dev)) //只要fifo 有数据就一直读
    {
        uint8_t byte;
        int recv_len = uart_fifo_read(dev, &byte, 1); //读一个字节

        if (recv_len <= 0)
        {
            break;
        }

        /* Strip \r to save BLE bandwidth */
        if (byte == '\r')
        {
            continue;//丢弃,省ble带宽
        }
       //写入环形缓冲区 ,判满   (牺牲一个槽位用来区分"空"和"满")
        uint16_t next = (ring_head + 1U) % RING_BUF_SIZE;
        if (next != ring_tail)
        {
            ring_buf[ring_head] = byte;
            ring_head = next;
        }
    }
}

/* ---------- BLE -> UART (phone sends command) ---------- */
//nus接收回调
/*`nus_received` 是 **GATT Write 的上层回调**。

BLE 底层实际发生的是：

1. 手机发 `ATT_WRITE_CMD`（Write Without Response）
2. nRF GATT 层把数据交给 NUS 服务
3. NUS 服务调用 `nus_received` 回调*/
static void nus_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint16_t index;
    bool has_newline = false;

    ARG_UNUSED(conn);
    LOG_HEXDUMP_INF(data, len, "BLE RX");

    for (index = 0U; index < len; index++)
    {
        uint8_t byte = data[index];
        // 过滤非打印字符（只放行 0x20~0x7E 和 CR/LF）
        //ASCII 可打印字符范围（空格到 ~
        if ((byte < 0x20U) && (byte != '\r') && (byte != '\n'))
        {
            continue;
        }
        if (byte > 0x7EU)
        {
            continue;
        }
      // 直接阻塞发给 STM32
        if (byte =
        uart_poll_out(uart_dev, byte);
        if (byte == '\n')
        {
            has_newline = true;
        }
    }
  // 如果这包没有换行，自动补一个
    if (!has_newline && (len > 0U))
    {
        uart_poll_out(uart_dev, '\n');
    }
}

static struct bt_nus_cb nus_cb = {
    .received = nus_received,
};


//连接管理回调

/*Zephyr BLE 对连接句柄做了**引用计数管理**：

- bt_conn_ref() = 引用计数 +1，防止协议栈在你还用的时候释放它
- bt_conn_unref() = 引用计数 -1，用完后归还，让协议栈可以回收

如果不调 bt_conn_ref，协议栈可能在某个时机把这块内存回收掉，`current_conn` 就成野指针了。

BT_CONN_CB_DEFINE是 Zephyr 提供的宏，用来注册 GAP 连接事件回调。
底层对应 GAP Connection Complete 和 GAP Disconnect 事件。*/
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err != 0U)
    {
        LOG_ERR("BLE connect failed: %u", err);
        return;
    }
// 增加引用计数，保存连接句柄
    current_conn = bt_conn_ref(conn);
    LOG_INF("BLE connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(reason);

    if (current_conn != NULL)
    {
        // 减少引用计数，释放连接句柄
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    LOG_INF("BLE disconnected");
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};


//初始化
static int uart_bridge_init(void)
{
    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("uart0 not ready");
        return -ENODEV;
    }
 //注册ISR
    uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
//开RX中断
    uart_irq_rx_enable(uart_dev);
// UART 每收到字节就自动触发 uart_isr
    return 0;
}

static int ble_bridge_init(void)
{
    int err;
 // 1. 启动 BLE 协议栈（包括 SoftDevice/Controller）
/* - 初始化 BLE 控制器（nRF52832 内置的射频硬件）
- 初始化 L2CAP、ATT、GAP、GATT 各层协议栈
- 参数 `NULL` 表示同步等待初始化完成*/
    err = bt_enable(NULL);
    if (err != 0)
    {
        LOG_ERR("bt_enable failed: %d", err);
        return err;
    }
// 2. 注册 NUS 服务（向 GATT 数据库添加 Service）
/*  - 向 GATT 数据库注册 NUS Service（含 TX/RX 两个 Characteristic）
    - 绑定 `received` 回调到 RX Characteristic 的 Write 事件*/
    err = bt_nus_init(&nus_cb);
    if (err != 0)
    {
        LOG_ERR("bt_nus_init failed: %d", err);
        return err;
    }
// 3. 开始广播
/* - `BT_LE_ADV_CONN_FAST_1` = 快速广播模式（间隔约 30ms，持续 30s 后自动切慢速）
 - 启动后每隔约 30ms 发一次广播包，手机扫描就能发现 `Mood_EQ_Bridge`*/
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err != 0)
    {
        LOG_ERR("Advertising start failed: %d", err);
        return err;
    }

    LOG_INF("Advertising started");
    return 0;
}

/* ---------- Main loop: drain ring buffer -> BLE ---------- */

//主循环
int main(void)
{
    LOG_INF("Mood EQ BLE bridge start");
    LOG_INF("UART wiring: P0.06 -> STM32 PA3, P0.08 <- STM32 PA2");

    if (uart_bridge_init() != 0)
    {
        return -1;
    }

    if (ble_bridge_init() != 0)
    {
        return -1;
    }

    while (1)
    {
        uint16_t count = ring_count();

        if (count > 0U)
        {  // 等 STM32 把这一帧全发完
            /* Wait until STM32 finishes sending (no new bytes for 5ms) */
            uint16_t prev;
            do {
                prev = count;
                k_msleep(5);
                count = ring_count();
            } while (count != prev);

            // 按 20 字节分片发给手机
            /* Drain ring buffer in BLE_CHUNK_SIZE chunks */
            while ((ring_count() > 0U) && (current_conn != NULL))
            {
                uint8_t chunk[BLE_CHUNK_SIZE];
                uint16_t n = 0U;

                while ((n < BLE_CHUNK_SIZE) && (ring_head != ring_tail))
                {
                    chunk[n++] = ring_buf[ring_tail];
                    ring_tail = (ring_tail + 1U) % RING_BUF_SIZE;
                }

                ble_send_chunk(chunk, n);

                if (ring_count() > 0U)
                { // 给 BLE 协议栈喘气
                    //- BLE 连接间隔通常 7.5~100ms
                    // - 如果发太快，协议栈队列会满，返回 `EAGAIN`
                    k_msleep(15);
                }
            }
        }

        k_msleep(5);//主循环节拍
    }
}