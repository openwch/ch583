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
#define SBP_START_DEVICE_EVT    0x0001

#define SBP_READ_RSSI_EVT       0x0004
#define SBP_PARAM_UPDATE_EVT    0x0008
#define UART_TO_BLE_SEND_EVT    0x0010

// Simple Profile Service UUID
#define SIMPLEPROFILE_SERV_UUID     0xFFE0
/*********************************************************************
 * MACROS
 */
typedef struct
{
    uint16 connHandle; // Connection handle of current connection
    uint16 connInterval;
    uint16 connSlaveLatency;
    uint16 connTimeout;
} peripheralConnItem_t;

extern uint8_t Peripheral_TaskID;
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

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
