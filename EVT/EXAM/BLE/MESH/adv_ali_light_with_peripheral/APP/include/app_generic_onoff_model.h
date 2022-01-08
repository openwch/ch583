/********************************** (C) COPYRIGHT *******************************
* File Name          : app_generic_onoff_model.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 
*******************************************************************************/

#ifndef app_generic_onoff_model_H
#define app_generic_onoff_model_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "MESH_LIB.h"
#define MSG_PIN			GPIO_Pin_12

BOOL read_led_state(u32_t led_pin);

extern const struct bt_mesh_model_op gen_onoff_op[];
void set_led_state(u32_t led_pin, BOOL on);

void toggle_led_state(u32_t led_pin);


#ifdef __cplusplus
}
#endif

#endif
