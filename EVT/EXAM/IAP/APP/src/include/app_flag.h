/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_flag.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/03/15
 * Description        : USB IAP APP例程
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef _APP_FLAG_H_
#define _APP_FLAG_H_

#include "CH58x_common.h"

void SwitchImageFlag(uint8_t new_flag);
void jumpToIap(void);

#define FLAG_USER_CALL_IAP   0x55
#define FLAG_USER_CALL_APP   0xaa

/* 存放在DataFlash地址，不能占用蓝牙的位置 */
#define IAP_FLAG_DATAFLASH_ADD               0

/* 存放在DataFlash里的OTA信息 */
typedef struct
{
    unsigned char ImageFlag;            //记录的当前的image标志
    unsigned char Revd[3];
} IAPDataFlashInfo_t;


#endif /* _APP_FLAG_H_ */
