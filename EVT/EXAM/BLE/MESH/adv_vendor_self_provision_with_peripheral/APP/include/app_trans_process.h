/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_trans_process.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/03/31
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_trans_process_H
#define app_trans_process_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#define LED_PIN    GPIO_Pin_18

/******************************************************************************/
/**
 * @brief   读取led状态
 *
 * @param   led_pin - 引脚
 *
 * @return  led状态
 */
BOOL read_led_state(uint32_t led_pin);

/**
 * @brief   设置led状态
 *
 * @param   led_pin - 引脚
 * @param   on      - 状态
 */
void set_led_state(uint32_t led_pin, BOOL on);

/**
 * @brief   翻转led状态
 *
 * @param   led_pin - 引脚
 */
void toggle_led_state(uint32_t led_pin);

/**
 * @brief   处理trans数据
 *
 * @param   pValue      - 数据指针.
 *          len         - 数据长度.
 *          src_Addr    - 数据来源地址.
 *          dst_Addr    - 数据目的地址.
 */
extern void app_trans_process(uint8_t *pValue, uint8_t len, uint16_t src_Addr, uint16_t dst_Addr);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
