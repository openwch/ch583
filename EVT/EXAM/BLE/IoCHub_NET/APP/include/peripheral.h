/********************************** (C) COPYRIGHT *******************************
 * File Name          : peripheral.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Peripheral Task Events
#define SBP_START_DEVICE_EVT             0x0001
#define SBP_READ_RSSI_EVT                   0x0004
#define SBP_PARAM_UPDATE_EVT          0x0008
#define IOCHUB_TO_BLE_SEND_EVT       0x0010
#define IOCHUB_DATA_PROCESS_EVT    0x0020
#define IOCHUB_ALL_DP_UPLOAD_EVT  0x0040
#define IOCHUB_SWITCH_CHANGE_EVT  0x0080

/*********************************************************************
 * MACROS
 */
#define LED_LINK_PIN    GPIO_Pin_18
#define LED_LIGHT_PIN GPIO_Pin_19
#define KEY_SWITCH_PIN  GPIO_Pin_4

/*===============================DP数据点类型===========================================*/
#define         DP_TYPE_RAW                     0x00        //RAW 类型
#define         DP_TYPE_BOOL                    0x01        //bool 类型
#define         DP_TYPE_VALUE                   0x02        //value 类型
#define         DP_TYPE_STRING                  0x03        //string 类型
#define         DP_TYPE_ENUM                    0x04        //enum 类型
#define         DP_TYPE_BITMAP                  0x05        //fault 类型

/*=============================BLE Packet Protocol Format=================================*/
#define BLE_REC_FST          0xBB
#define BLE_REC_SEC         0x44

#define BLE_SEND_FST        0x33
#define BLE_SEND_SEC        0xCC

#define BLE_HEAD_LEN        2
#define BLE_FOMRAT_LEN   4
#define BLE_TAIL_LEN          1

#define DP_ID_LIGHT           0x01      //灯控

#define DP_ID_SWITCH        0x02      //开关




#define IOCHUB_DEVICE_TYPE_INDEX    0
#define IOCHUB_DEVICE_TYPE_LEN     2
#define IOCHUB_DEVICE_TYPE_BLE      0x0001
#define IOCHUB_DEVICE_TYPE_NET     0x0002

#define IOCHUB_INTF_TYPE_INDEX    2

#define IOCHUB_TYPE_NAME   0x01
#define IOCHUB_TYPE_MAC     0x02
#define IOCHUB_TYPE_MTU     0x03





extern uint8_t Peripheral_TaskID; // Task ID for internal task/event processing
/*================================================================================*/
typedef struct
{
    uint16 connHandle; // Connection handle of current connection
    uint16 connInterval;
    uint16 connSlaveLatency;
    uint16 connTimeout;
} peripheralConnItem_t;


typedef struct {
    uint8_t *data;
    uint16_t length;
    uint16_t capacity;
}buffer_t;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Peripheral_Init(void);

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 Peripheral_ProcessEvent(uint8 task_id, uint16 events);

extern uint8_t ble_send_frame(uint8_t frame_cmd, uint8_t frame_type, uint8_t* frame_buf, uint8_t frame_dlc);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
