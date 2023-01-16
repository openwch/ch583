/********************************** (C) COPYRIGHT *******************************
 * File Name          : runningsensor.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef RUNNINGSENSOR_H
#define RUNNINGSENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Running Sensor Task Events
#define START_DEVICE_EVT             0x0001
#define RSC_PERIODIC_EVT             0x0002
#define RSC_CONN_PARAM_UPDATE_EVT    0x0004
#define RSC_NEGLECT_TIMEOUT_EVT      0x0008

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void RunningSensor_Init(void);

/*
 * Task Event Processor for the BLE Application
 */
extern uint16_t RunningSensor_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RUNNINGSENSOR_H */
