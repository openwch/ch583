/********************************** (C) COPYRIGHT *******************************
 * File Name          : rf_phy.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef CENTRAL_H
#define CENTRAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define SBP_RF_START_DEVICE_EVT    1
#define SBP_RF_PERIODIC_EVT        2
#define SBP_RF_RF_RX_EVT           4

#define LLE_MODE_ORIGINAL_RX       (0x80) //如果配置LLEMODE时加上此宏，则接收第一字节为原始数据（原来为RSSI）

extern void RF_Init(void);

#ifdef __cplusplus
}
#endif

#endif
