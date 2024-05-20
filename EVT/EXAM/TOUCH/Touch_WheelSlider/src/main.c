/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2024/02/27
 * Description        : 外设从机应用主函数及任务系统初始化
 * Copyright (c) 2024 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "Touch.h"
#include "app.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

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
	SetSysClock(CLK_SOURCE_PLL_60MHz);
	DebugInit();
	PRINT("touch wheel slider\n");
    touch_init();
	while(1)
	{
		touch_dataProcess();
	}
}

/******************************** endfile @ main ******************************/
