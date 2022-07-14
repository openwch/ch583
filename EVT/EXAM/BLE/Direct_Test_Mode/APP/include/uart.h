/********************************** (C) COPYRIGHT *******************************
 * File Name          : uart.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/30
 * Description        : 
 * Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef BLE_DIRECTTEST_APP_INCLUDE_UART_H
#define BLE_DIRECTTEST_APP_INCLUDE_UART_H

#include "config.h"
#include "buf.h"

#define UART_PROCESS_EVT                1

/*process events*/
#define UART_RECEIVE_POLL_EVT           (1<<0)

struct uart_process_msg {
    tmos_event_hdr_t hdr;
    uint8_t *data;
};


int uart_send(struct simple_buf *buf);
int uart_start_receiving(void);
void uart_task_init(void);

#endif /* BLE_DIRECTTEST_APP_INCLUDE_UART_H */
