/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_generic_onoff2_model.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_generic_onoff2_model_H
#define app_generic_onoff2_model_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#define MSG2_PIN    GPIO_Pin_13

BOOL read_led2_state(uint32_t led_pin);

extern const struct bt_mesh_model_op gen_onoff2_op[];

void set_led2_state(uint32_t led_pin, BOOL on);

void toggle_led2_state(uint32_t led_pin);

#ifdef __cplusplus
}
#endif

#endif
