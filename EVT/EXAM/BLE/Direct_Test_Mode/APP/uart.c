/********************************** (C) COPYRIGHT *******************************
 * File Name          : uart.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/30
 * Description        : 
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"
#include "config.h"
#include "uart.h"
#include "atomic.h"
#include "test_dtm.h"

static tmosTaskID uart_taskid;

static uint8_t rx_buf[100];
static struct simple_buf *uart_buf = NULL;
static struct simple_buf uart_buffer;

enum uart_status {
    UART_STATUS_IDLE,
    UART_STATUS_RCVING,
    UART_STATUS_RCV_END,
    UART_STATUS_SENDING,

    UART_STATUS_NUM,
};

atomic_t uart_flag;

void uart_buffer_create(struct simple_buf **buf)
{
    *buf = simple_buf_create(&uart_buffer, rx_buf, sizeof(rx_buf));
}

int uart_start_receiving(void)
{
    PRINT("uart0 reeciving\n");
    atomic_set(&uart_flag, UART_STATUS_RCVING);
    uart_buffer_create(&uart_buf);
    PFIC_EnableIRQ(UART0_IRQn);

    return 0;
}

int uart_send(struct simple_buf *buf)
{
    atomic_set(&uart_flag, UART_STATUS_SENDING);

    uint8_t *send_data;
    uint16_t send_len;

    send_len = buf->len;
    send_data = simple_buf_pull(buf, buf->len);

    UART0_SendString(send_data, send_len);
    atomic_set(&uart_flag, UART_STATUS_IDLE);

    PRINT("uart0 send %d bytes [", send_len);
    for(int i = 0; i < send_len; i++){
        if(i) PRINT(" ");
        PRINT("%#x", send_data[i]);
    }
    PRINT("]\n");

    return 0;
}

/*********************************************************************
 * @fn      uart_processevent
 *
 * @brief   process handle
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
tmosEvents uart_processevent(tmosTaskID task_id, tmosEvents events)
{
    if(events & SYS_EVENT_MSG)
    { // 处理HAL层消息，调用tmos_msg_receive读取消息，处理完成后删除消息。
        uint8_t *msgPtr;

        msgPtr = tmos_msg_receive(task_id);
        if(msgPtr)
        {
            /* De-allocate */
            tmos_msg_deallocate(msgPtr);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & UART_RECEIVE_POLL_EVT) {
        if(atomic_get(&uart_flag) == UART_STATUS_RCV_END) {
            PRINT("uart0 recevied %d bytes [", uart_buf->len);
            for(int i = 0; i < uart_buf->len; i++) {
                if(i) PRINT(" ");
                PRINT("%#x", uart_buf->data[i]);
            }
            PRINT("]\n");

            struct uart_process_msg *uart_msg;

            uart_msg = (struct uart_process_msg *)    \
                tmos_msg_allocate(sizeof(struct uart_process_msg));

            if(uart_msg) {
                uart_msg->hdr.event = UART_PROCESS_EVT;
                uart_msg->hdr.status = true;
                uart_msg->data = (uint8_t *)uart_buf;
                tmos_msg_send(test_taskid, (uint8 *)uart_msg );
            }
        }

        return (events ^ UART_RECEIVE_POLL_EVT);
    }

    return 0;
}


void uart_task_init(void)
{
    uart_taskid = TMOS_ProcessEventRegister(uart_processevent);

    GPIOB_SetBits(bTXD0);
    GPIOB_ModeCfg(bTXD0, GPIO_ModeOut_PP_5mA);
    GPIOB_ModeCfg(bRXD0, GPIO_ModeIN_PU);

    UART0_DefInit();
    UART0_ByteTrigCfg(UART_7BYTE_TRIG);
    UART0_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
    tmos_start_reload_task(uart_taskid, UART_RECEIVE_POLL_EVT, 
        MS1_TO_SYSTEM_TIME(100));

    atomic_set(&uart_flag, UART_STATUS_IDLE);
    uart_start_receiving();
}

/*********************************************************************
 * @fn      UART0_IRQHandler
 *
 * @brief   UART0中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void UART0_IRQHandler(void)
{
    volatile uint8_t i;

    switch(UART0_GetITFlag())
    {
        case UART_II_LINE_STAT: // 线路状态错误
        {
            (void)UART0_GetLinSTA();
            break;
        }

        case UART_II_RECV_RDY: // 数据达到设置触发点
            for(i = 0; i < (7 - 1); i++)
            {
                simple_buf_add_u8(uart_buf, UART0_RecvByte());
            }
            break;

        case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
            while(R8_UART0_RFC)
            {
                simple_buf_add_u8(uart_buf, UART0_RecvByte());
            }
            atomic_set(&uart_flag, UART_STATUS_RCV_END);
            PFIC_DisableIRQ(UART0_IRQn);
            break;

        case UART_II_THR_EMPTY: // 发送缓存区空，可继续发送
            break;

        case UART_II_MODEM_CHG: // 只支持串口0
            break;

        default:
            break;
    }
}
