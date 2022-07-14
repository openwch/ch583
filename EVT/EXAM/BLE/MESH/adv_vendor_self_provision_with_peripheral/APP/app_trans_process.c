/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_trans_process.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/01/18
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
#include "CONFIG.h"
#include "MESH_LIB.h"
#include "app_vendor_model_srv.h"
#include "app.h"
#include "peripheral.h"
#include "HAL.h"
#include "app_trans_process.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * LOCAL FUNCION
 */

/*********************************************************************
 * GLOBAL TYPEDEFS
 */


/*********************************************************************
 * @fn      read_led_state
 *
 * @brief   获取当前灯状态.
 *
 * @param   led_pin - 引脚.
 *
 * @return  灯状态
 */
BOOL read_led_state(uint32_t led_pin)
{
    return (GPIOB_ReadPortPin(led_pin) > 0) ? 0 : 1;
}

/*********************************************************************
 * @fn      set_led_state
 *
 * @brief   设置当前灯状态.
 *
 * @param   led_pin - 引脚.
 * @param   on      - 状态.
 *
 * @return  none
 */
void set_led_state(uint32_t led_pin, BOOL on)
{
    GPIOB_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);
    on ? GPIOB_ResetBits(led_pin) : GPIOB_SetBits(led_pin);
}

/*********************************************************************
 * @fn      toggle_led_state
 *
 * @brief   翻转当前灯状态
 *
 * @param   led_pin - 引脚.
 *
 * @return  none
 */
void toggle_led_state(uint32_t led_pin)
{
    GPIOB_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);
    GPIOB_InverseBits(led_pin);
}

/*********************************************************************
 * @fn      app_trans_process
 *
 * @brief   处理收到的trans数据
 *
 * @param   pValue      - 数据指针.
 *          len         - 数据长度.
 *          src_Addr    - 数据来源地址.
 *          dst_Addr    - 数据目的地址.
 *
 * @return  none
 */
void app_trans_process(uint8_t *pValue, uint8_t len, uint16_t src_Addr, uint16_t dst_Addr)
{
    uint16_t opcode = (pValue[0]<<8)|pValue[1];
    switch( opcode )
    {
        case BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
        {
            set_led_state(LED_PIN, pValue[2]);
            break;
        }

        default:
        {
            break;
        }
    }

}



/******************************** endfile @ main ******************************/
