/********************************** (C) COPYRIGHT *******************************
 * File Name          : observer.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        : The main function and task system of the observer application initialization
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef OBSERVER_H
#define OBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Simple BLE Observer Task Events
#define START_DEVICE_EVT       0x0001
#define START_DISCOVERY_EVT    0x0002
#define START_SCAN_EVT         0x0004

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Observer_Init(void);

/*
 * Task Event Processor for the BLE Application
 */
extern uint16_t Observer_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OBSERVER_H */
