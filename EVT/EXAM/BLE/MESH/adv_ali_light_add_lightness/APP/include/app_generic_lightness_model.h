/********************************** (C) COPYRIGHT *******************************
* File Name          : app_generic_lightness_model.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 
*******************************************************************************/

#ifndef app_generic_lightness_model_H
#define app_generic_lightness_model_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "MESH_LIB.h"
#define MSG_PIN			GPIO_Pin_12


u16_t read_led_lightness(u32_t led_pin);

extern const struct bt_mesh_model_op gen_lightness_op[];
void set_led_lightness(u32_t led_pin, u16_t lightness);



#ifdef __cplusplus
}
#endif

#endif
