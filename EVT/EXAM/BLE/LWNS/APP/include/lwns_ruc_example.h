/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_ruc_example.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/30
 * Description        : reliable unicast，可靠单播传输例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef _LWNS_RUC_EXAMPLE_H_
#define _LWNS_RUC_EXAMPLE_H_

#include "lwns_config.h"

#define RUC_EXAMPLE_TX_PERIOD_EVT    1 << (0)

void lwns_ruc_process_init(void);

#endif /* _LWNS_RUC_EXAMPLE_H_ */
