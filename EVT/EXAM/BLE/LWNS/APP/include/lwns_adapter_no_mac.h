/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_adapter_no_mac.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/20
 * Description        : lwns适配器，不使用mac协议，纯透传
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef _LWNS_ADAPTER_NO_MAC_H_
#define _LWNS_ADAPTER_NO_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lwns_config.h"

#define LWNS_USE_NO_MAC    0  //是否使能纯透传mac协议，适合不存在并发情况的星型网络，即主机问从机答的星型网络。

#if LWNS_USE_NO_MAC

typedef enum
{
    BLE_PHY_MANAGE_STATE_FREE = 0,
    BLE_PHY_MANAGE_STATE_SENDING,
} BLE_PHY_MANAGE_STATE_t;

  #define LLE_MODE_ORIGINAL_RX          (0x80) //如果配置LLEMODE时加上此宏，则接收第一字节为原始数据（原来为RSSI）

  #define LWNS_HTIMER_PERIOD_MS         20    //为(1000/HTIMER_SECOND_NUM)

    //RF_TX和RF_RX所用的类型，可以修改，不推荐改
  #define USER_RF_RX_TX_TYPE            0xff

  #define LWNS_PHY_OUTPUT_TIMEOUT_MS    5

    //receive process evt
  #define LWNS_PHY_RX_OPEN_EVT          1
    //send process evt
  #define LWNS_HTIMER_PERIOD_EVT        1
  #define LWNS_PHY_OUTPUT_FINISH_EVT    2

extern void RF_Init(void);

extern void lwns_init(void);

extern void lwns_shut(void);

extern void lwns_start(void);

#endif /* LWNS_USE_NO_MAC */

#ifdef __cplusplus
}
#endif

#endif /* _LWNS_ADAPTER_NO_MAC_H_ */
