/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_uninetflood_example.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/10/25
 * Description        : lwns单播网络泛洪传输例子，将数据网络泛洪发送至指定节点
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef _LWNS_UNINETFLOOD_EXAMPLE_H_
#define _LWNS_UNINETFLOOD_EXAMPLE_H_

#include "lwns_config.h"

#define UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT    1 << (0)

void lwns_uninetflood_process_init(void);

#endif /* _LWNS_UNINETFLOOD_EXAMPLE_H_ */
