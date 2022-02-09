/********************************** (C) COPYRIGHT *******************************
 * File Name          : directtest.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 直接测试程序，测试指定通信频道发送数据包
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "directtest.h"

static tmosTaskID testTaskID;
#if BLE_DIRECT_TEST
static uint8_t TestEnalbe = FALSE;
#endif

static uint8_t payload = 0;

/*********************************************************************
 * @fn      TEST_ProcessEvent
 *
 * @brief   事务处理
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
tmosEvents TEST_ProcessEvent(tmosTaskID task_id, tmosEvents events)
{
    uint8_t     *msgPtr;
    keyChange_t *msgKeyPtr;

    if(events & SYS_EVENT_MSG)
    { // 处理HAL层消息，调用tmos_msg_receive读取消息，处理完成后删除消息。
        msgPtr = tmos_msg_receive(task_id);
        if(msgPtr)
        {
            /* De-allocate */
            tmos_msg_deallocate(msgPtr);
        }
        return events ^ SYS_EVENT_MSG;
    }
    if(events & TEST_EVENT)
    {
        if(TestEnalbe == FALSE)
        {
            uint8 pParm[4];
            payload++;
            TestEnalbe = TRUE;
            HalLedBlink(HAL_LED_1, 0xff, 30, 4000);
            /*TX_Channel*/
            pParm[0] = 0;
            /*Test_Data_Length*/
            pParm[1] = 37;
            /*Packet_Payload
             * 0x00 PRBS9 sequence '11111111100000111101…'
             * 0x01 Repeated '11110000'
             * 0x02 Repeated '10101010'
             * 0x03 PRBS15
             * 0x04 Repeated '11111111'
             * 0x05 Repeated '00000000'
             * 0x06 Repeated '00001111'
             * 0x07 Repeated '01010101'*/
            pParm[2] = payload;
            /*PHY
             * 0x01 LE 1M PHY
             * 0x02 LE 2M PHY
             * 0x03 LE Coded PHY with S=8 data coding
             * 0x04 LE Coded PHY with S=2 data coding*/
            pParm[3] = 1;
            API_LE_TransmitterTestCmd(pParm, 0x2034);
            tmos_start_task(testTaskID, TEST_EVENT, MS1_TO_SYSTEM_TIME(20 * 1000)); // 测试时间20s
            PRINT("test start ...\n");
        }
        else
        {
            TestEnalbe = FALSE;
            HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
            API_LE_TestEndCmd();
            PRINT("   end!\n");
        }
        return events ^ TEST_EVENT;
    }
    return 0;
}

/*********************************************************************
 * @fn      key_Change
 *
 * @brief   按键回调
 *
 * @param   keys    - 按键类型.
 *
 * @return  none
 */
void key_Change(uint8 keys)
{
    if(keys & HAL_KEY_SW_1)
    {
        if(TestEnalbe == FALSE)
        {
            uint8 pParm[4];
            payload++;
            TestEnalbe = TRUE;
            HalLedBlink(HAL_LED_1, 0xff, 30, 4000);
            pParm[0] = 0;
            pParm[1] = 37;
            pParm[2] = payload;
            pParm[3] = 1;
            API_LE_TransmitterTestCmd(pParm, 0x2034);
            PRINT("(key)test start ...\n");
        }
        else
        {
            TestEnalbe = FALSE;
            HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
            API_LE_TestEndCmd();
            PRINT("   (key)end!\n");
        }
        tmos_stop_task(testTaskID, TEST_EVENT);
    }
}

/*********************************************************************
 * @fn      TEST_Init
 *
 * @brief   初始化
 *
 * @return  none
 */
void TEST_Init(void)
{
    testTaskID = TMOS_ProcessEventRegister(TEST_ProcessEvent);
#if(defined HAL_KEY) && (HAL_KEY == TRUE)
    HalKeyConfig(key_Change);
#endif
#if BLE_DIRECT_TEST
    tmos_start_task(testTaskID, TEST_EVENT, 1000); // 添加一个测试任务
#endif
}
/******************************** endfile @ mcu ******************************/
