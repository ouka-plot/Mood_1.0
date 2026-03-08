#ifndef __EQ_CTRL_UART_H
#define __EQ_CTRL_UART_H

#include "./SYSTEM/SYS/sys.h"

void eq_ctrl_uart_init(uint32_t baudrate);
void eq_ctrl_uart_poll(void);
void eq_ctrl_uart_send_text(const char *text);

#endif