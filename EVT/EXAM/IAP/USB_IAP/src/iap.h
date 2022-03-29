/********************************** (C) COPYRIGHT *******************************
 * File Name          : iap.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/03/15
 * Description        : USB IAP例程
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/
#ifndef _IAP_H_
#define _IAP_H_

#include "CH58x_common.h"

/* you can change the following definitions below, just keep them same in app and iap. */
#define        APP_CODE_START_ADDR        0x00001000
#define        APP_CODE_END_ADDR          0x00070000


#define     USE_EEPROM_FLAG     1

#define    jumpApp   ((  void  (*)  ( void ))  ((int*)APP_CODE_START_ADDR))


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

/* you should not change the following definitions below. */
#define        CMD_IAP_PROM         0x80
#define        CMD_IAP_ERASE        0x81
#define        CMD_IAP_VERIFY       0x82
#define        CMD_IAP_END          0x83

/* usb data length is 64 */
#define     IAP_LEN            64

typedef union _IAP_CMD_STRUCT
{
    struct
    {
        uint8_t    cmd;
        uint8_t    len;
        uint8_t    addr[4];
    } erase;
    struct
    {
    	uint8_t    cmd;
    	uint8_t    len;
    	uint8_t    status[2];
    } end;
    struct
    {
    	uint8_t    cmd;
    	uint8_t    len;
    	uint8_t    addr[4];
    	uint8_t    buf[IAP_LEN - 6];
    } verify;
    struct
    {
    	uint8_t    cmd;
    	uint8_t    len;
    	uint8_t    buf[IAP_LEN - 2];
    } program;
    struct
    {
    	uint8_t    buf[IAP_LEN];
    } other;
} iap_cmd_t;

extern uint8_t EP0_Databuf[64 + 64 + 64];    //ep0(64)+ep4_out(64)+ep4_in(64)
extern uint8_t EP1_Databuf[64 + 64];    //ep1_out(64)+ep1_in(64)
extern uint8_t EP2_Databuf[64 + 64];    //ep2_out(64)+ep2_in(64)
extern uint8_t EP3_Databuf[64 + 64];    //ep3_out(64)+ep3_in(64)
extern uint32_t g_tcnt;

extern void my_memcpy(void *dst, const void *src, uint32_t l);

extern void USB_DevTransProcess(void);

#endif /* _IAP_H_ */
