/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2024/02/27
 * Description        : 触摸按键结合蓝牙外设例程
 * Copyright (c) 2024 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "gattprofile.h"
#include "peripheral.h"
#include "Touch.h"
#include "app_tmos.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
	while(1)
	{
		TMOS_SystemProcess();
	}
}

/*********************************************************************
 * @fn      DebugInit
 *
 * @brief   串口打印初始化
 *
 * @return  none
 *
 * @note	Uart1的默认映射引脚可能与触摸通道冲突，
 * 			在触摸应用中应选用其他UART或将引脚重映射
 */
void DebugInit(void)
{
#ifdef  DEBUG
#if DEBUG == Debug_UART0
	GPIOB_SetBits(GPIO_Pin_7);
	GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
	UART0_DefInit();
#elif DEBUG == Debug_UART1
	GPIOPinRemap(ENABLE, RB_PIN_UART1);
	GPIOB_SetBits(GPIO_Pin_13);
	GPIOB_ModeCfg(GPIO_Pin_13, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
#elif DEBUG == Debug_UART2
	GPIOPinRemap(ENABLE, RB_PIN_UART2);
	GPIOB_SetBits(GPIO_Pin_23);
	GPIOB_ModeCfg(GPIO_Pin_23, GPIO_ModeOut_PP_5mA);
	UART2_DefInit();
#elif DEBUG == Debug_UART3
	GPIOPinRemap(ENABLE, RB_PIN_UART3);
	GPIOB_SetBits(GPIO_Pin_21);
	GPIOB_ModeCfg(GPIO_Pin_21, GPIO_ModeOut_PP_5mA);
	UART3_DefInit();
#endif
#endif
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
	PWR_DCDCCfg(ENABLE);
#endif
	SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
	GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
	GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

	DebugInit();
	PRINT("12 touch keys with BLE\n");
	PRINT("%s\n", VER_LIB);

	CH58X_BLEInit();
	HAL_Init();
	GAPRole_PeripheralInit();
	Peripheral_Init();
	touch_on_TMOS_init();
	Main_Circulation();
}

/******************************** endfile @ main ******************************/
