/********************************** (C) COPYRIGHT *******************************
 * File Name          : iap.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/03/15
 * Description        : UART IAP例程
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef _IAP_H_
#define _IAP_H_

#include "CH58x_common.h"

/* you can change the following definitions below, just keep them same in app and iap. */
#define        APP_CODE_START_ADDR        0x00001000
#define        APP_CODE_END_ADDR          0x00070000

#define     USE_EEPROM_FLAG     0

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
        UINT8    sop1;
        UINT8    sop2;
        UINT8    cmd;
        UINT8    len;
        UINT32   addr;
        UINT16   checkNum;
        UINT8    eop1;
        UINT8    eop2;
    } erase;
    struct
    {
        UINT8    sop1;
        UINT8    sop2;
        UINT8    cmd;
        UINT8    len;
        UINT16   checkNum;
        UINT8    eop1;
        UINT8    eop2;
    } end;
    struct
    {
        UINT8    sop1;
        UINT8    sop2;
        UINT8    cmd;
        UINT8    len;
        UINT32   addr;
        UINT8    data[IAP_LEN];
        UINT16   checkNum;
        UINT8    eop1;
        UINT8    eop2;
    } verify;
    struct
    {
        UINT8    sop1;
        UINT8    sop2;
        UINT8    cmd;
        UINT8    len;
        UINT8    data[IAP_LEN];
        UINT16   checkNum;
        UINT8    eop1;
        UINT8    eop2;
    } program;
    struct
    {
        UINT8    buf[IAP_LEN + 12];
    } other;
} iap_cmd_t;

extern uint32_t g_tcnt;

extern void my_memcpy(void *dst, const void *src, uint32_t l);

extern void Main_Circulation();

#define IAP_DATA_SOP1     0xaa
#define IAP_DATA_SOP2     0x55

#define IAP_DATA_EOP1     0x55
#define IAP_DATA_EOP2     0xaa

typedef enum
{
    IAP_ERR_UNKNOWN = 0,/* 命令未知，不可能出现 */
    IAP_ERR_OVERTIME,/* 每次串口有数据后，超过10个字符时间没有新数据，并且也解析不到一个合法的数据包，即报错 */
    IAP_ERR_CHECK,/* 一个合法的数据包中校验和不通过，即报错 */
    IAP_ERR_ADDR,/* 擦除时，地址不对 */
    IAP_ERR_ERASE_FAIL,/* 擦除失败 */
    IAP_ERR_PROG_NO_ERASE,/* 没有先擦除，再写入 */
    IAP_ERR_WRITE_FAIL,/* 没有写入失败 */
    IAP_ERR_VERIFY,/* 校验失败 */
} IAP_ERR_t;

typedef enum
{
    IAP_DATA_REC_STATE_WAIT_SOP1 = 0,
    IAP_DATA_REC_STATE_WAIT_SOP2,
    IAP_DATA_REC_STATE_WAIT_CMD,
    IAP_DATA_REC_STATE_WAIT_LEN,
    IAP_DATA_REC_STATE_WAIT_ADDR,
    IAP_DATA_REC_STATE_WAIT_DATA,
    IAP_DATA_REC_STATE_WAIT_CHECKNUM,
    IAP_DATA_REC_STATE_WAIT_EOP1,
    IAP_DATA_REC_STATE_WAIT_EOP2,
    IAP_DATA_REC_STATE_OK,
} IAP_DATA_REC_STATE_t;



#endif /* _IAP_H_ */
