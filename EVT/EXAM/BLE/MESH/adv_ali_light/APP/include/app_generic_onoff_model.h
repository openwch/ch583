/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_generic_onoff_model.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2021/11/18
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_generic_onoff_model_H
#define app_generic_onoff_model_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

#define MSG_PIN    GPIO_Pin_12

extern const struct bt_mesh_model_op gen_onoff_op[];

/**
 * @brief   获取当前灯状态.
 *
 * @param   led_pin - 引脚.
 *
 * @return  灯状态
 */
BOOL read_led_state(uint32_t led_pin);

/**
 * @brief   设置当前灯状态.
 *
 * @param   led_pin - 引脚.
 * @param   on      - 状态.
 */
void set_led_state(uint32_t led_pin, BOOL on);

/**
 * @brief   翻转当前灯状态
 *
 * @param   led_pin - 引脚.
 */
void toggle_led_state(uint32_t led_pin);

#ifdef __cplusplus
}
#endif

#endif
