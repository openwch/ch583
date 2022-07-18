/********************************** (C) COPYRIGHT *******************************
 * File Name          : test_dtm.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/29
 * Description        : 
 * Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"
#include "config.h"
#include "test_dtm.h"
#include "uart.h"

tmosTaskID test_taskid = INVALID_TASK_ID;

/*********************************************************************
 * @fn      test_dtm_process_msg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void test_dtm_process_msg(tmos_event_hdr_t *pMsg) {
    switch (pMsg->event) {
    case UART_PROCESS_EVT:{
        struct simple_buf *evt = NULL;
        struct uart_process_msg *msg = \
            (struct uart_process_msg *)pMsg; 

        evt = hci_cmd_handle((struct simple_buf*)msg->data);

        if(evt)
            uart_send(evt);

        uart_start_receiving();
    }
        break;

    default:
        break;
    }
}

/*********************************************************************
 * @fn      test_dtm_processevent
 *
 * @brief   process handle
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
tmosEvents test_dtm_processevent(tmosTaskID task_id, tmosEvents events)
{
    uint8_t     *msgPtr;

    if(events & SYS_EVENT_MSG)
    { // 处理HAL层消息，调用tmos_msg_receive读取消息，处理完成后删除消息。
        msgPtr = tmos_msg_receive(task_id);
        if(msgPtr)
        {
            test_dtm_process_msg((tmos_event_hdr_t *)msgPtr);

            /* De-allocate */
            tmos_msg_deallocate(msgPtr);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    return 0;
}

void test_dtm_init(void)
{
    test_taskid = TMOS_ProcessEventRegister(test_dtm_processevent);
}
