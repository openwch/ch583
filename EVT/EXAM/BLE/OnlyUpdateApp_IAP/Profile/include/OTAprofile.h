/********************************** (C) COPYRIGHT *******************************
 * File Name          : OTAprofile.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef OTAPROFILE_H
#define OTAPROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// OTA Profile通道Index定义
#define OTAPROFILE_CHAR         0

// OTA 服务的UUID定义
#define OTAPROFILE_SERV_UUID    0xFEE0

// OTA 通讯通道UUID定义
#define OTAPROFILE_CHAR_UUID    0xFEE1

// Simple Keys Profile Services bit fields
#define OTAPROFILE_SERVICE      0x00000001

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// 读写操作函数回调
typedef void (*OTAProfileRead_t)(unsigned char paramID);
typedef void (*OTAProfileWrite_t)(unsigned char paramID, unsigned char *p_data, unsigned char w_len);

typedef struct
{
    OTAProfileRead_t  pfnOTAProfileRead;
    OTAProfileWrite_t pfnOTAProfileWrite;
} OTAProfileCBs_t;

/*********************************************************************
 * API FUNCTIONS
 */

/**
 * @brief   OTA Profile初始化
 *
 * @param   services    - 服务控制字
 *
 * @return  初始化的状态
 */
bStatus_t OTAProfile_AddService(uint32_t services);

/**
 * @brief   OTA Profile读写回调函数注册
 *
 * @param   appCallbacks    - 函数结构体指针
 *
 * @return  函数执行状态
 */
bStatus_t OTAProfile_RegisterAppCBs(OTAProfileCBs_t *appCallbacks);

/**
 * @brief   OTA Profile通道发送数据
 *
 * @param   paramID     - OTA通道选择
 * @param   p_data      - 数据指针
 * @param   send_len    - 发送数据长度
 *
 * @return  函数执行状态
 */
bStatus_t OTAProfile_SendData(unsigned char paramID, unsigned char *p_data, unsigned char send_len);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
