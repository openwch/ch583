/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_multicast_example.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/20
 * Description        : single-hop multicast£¬×é²¥´«ÊäÀý×Ó
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef _LWNS_MULTICAST_EXAMPLE_H_
#define _LWNS_MULTICAST_EXAMPLE_H_

#include "lwns_config.h"

#define MULTICAST_EXAMPLE_TX_PERIOD_EVT    1 << (0)

void lwns_multicast_process_init(void);

#endif /* _LWNS_MULTICAST_EXAMPLE_H_ */
