#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(mood_eq_bridge, LOG_LEVEL_INF);

#define DEVICE_NAME                 CONFIG_BT_DEVICE_NAME
#define BLE_CHUNK_SIZE              20U
#define RING_BUF_SIZE               512U

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static struct bt_conn *current_conn;

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
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, sizeof(DEVICE_NAME) - 1),
};

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
        err = bt_nus_send(current_conn, data, len);
        if (err == 0)
        {
            return;
        }
        if (err == -EAGAIN)
        {
            k_msleep(10);
            continue;
        }
        if ((err != -ENOTCONN))
        {
            LOG_WRN("bt_nus_send failed: %d (len=%u)", err, len);
        }
        return;
    }
}

/* ---------- UART ISR: bytes -> ring buffer ---------- */
static void uart_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    if (!uart_irq_update(dev))
    {
        return;
    }

    while (uart_irq_rx_ready(dev))
    {
        uint8_t byte;
        int recv_len = uart_fifo_read(dev, &byte, 1);

        if (recv_len <= 0)
        {
            break;
        }

        /* Strip \r to save BLE bandwidth */
        if (byte == '\r')
        {
            continue;
        }

        uint16_t next = (ring_head + 1U) % RING_BUF_SIZE;
        if (next != ring_tail)
        {
            ring_buf[ring_head] = byte;
            ring_head = next;
        }
    }
}

/* ---------- BLE -> UART (phone sends command) ---------- */
static void nus_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint16_t index;
    bool has_newline = false;

    ARG_UNUSED(conn);
    LOG_HEXDUMP_INF(data, len, "BLE RX");

    for (index = 0U; index < len; index++)
    {
        uint8_t byte = data[index];

        /* Skip non-printable chars except space, CR, LF */
        if ((byte < 0x20U) && (byte != '\r') && (byte != '\n'))
        {
            continue;
        }
        if (byte > 0x7EU)
        {
            continue;
        }

        uart_poll_out(uart_dev, byte);
        if (byte == '\n')
        {
            has_newline = true;
        }
    }

    /* Auto-append newline if the BLE packet didn't end with one */
    if (!has_newline && (len > 0U))
    {
        uart_poll_out(uart_dev, '\n');
    }
}

static struct bt_nus_cb nus_cb = {
    .received = nus_received,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err != 0U)
    {
        LOG_ERR("BLE connect failed: %u", err);
        return;
    }

    current_conn = bt_conn_ref(conn);
    LOG_INF("BLE connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(reason);

    if (current_conn != NULL)
    {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    LOG_INF("BLE disconnected");
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static int uart_bridge_init(void)
{
    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("uart0 not ready");
        return -ENODEV;
    }

    uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
    uart_irq_rx_enable(uart_dev);
    return 0;
}

static int ble_bridge_init(void)
{
    int err;

    err = bt_enable(NULL);
    if (err != 0)
    {
        LOG_ERR("bt_enable failed: %d", err);
        return err;
    }

    err = bt_nus_init(&nus_cb);
    if (err != 0)
    {
        LOG_ERR("bt_nus_init failed: %d", err);
        return err;
    }

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
        {
            /* Wait until STM32 finishes sending (no new bytes for 5ms) */
            uint16_t prev;
            do {
                prev = count;
                k_msleep(5);
                count = ring_count();
            } while (count != prev);

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
                {
                    k_msleep(15);
                }
            }
        }

        k_msleep(5);
    }
}