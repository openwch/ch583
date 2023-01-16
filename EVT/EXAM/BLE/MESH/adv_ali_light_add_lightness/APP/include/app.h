/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_SILENT_ADV_EVT    (1 << 0)

/******************************************************************************/

/**
 * @brief   应用层初始化
 */
void App_Init(void);

/**
 * @brief   发送当前灯的状态给天猫精灵
 */
void send_led_state(void);

/**
 * @brief   发送复位事件给天猫精灵，发送完成后将清除配网状态，重置自身mesh网络
 */
void send_reset_indicate(void);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
