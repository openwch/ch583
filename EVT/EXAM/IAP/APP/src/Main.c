/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/03/15
 * Description        : USB IAP APP例程
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"
#include "app_flag.h"

/*********************************************************************
 * @fn      DebugInit
 *
 * @brief   调试初始化
 *
 * @return  none
 */
void DebugInit(void)
{
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main()
{
    uint16_t i = 0;
    uint8_t  s = 0;

    SetSysClock(CLK_SOURCE_PLL_60MHz);

    /* 配置串口调试 */
    DebugInit();
    PRINT("Start @ChipID=%02x\n", R8_CHIP_ID);
    /* app程序必须执行该语句，保证app更新失败时，下次依然运行IAP */
    SwitchImageFlag(FLAG_USER_CALL_APP);

    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);
    while (1)
    {
		PRINT("i:%d\n",i);
		i++;
		DelayMs(10);
		if (GPIOB_ReadPortPin(GPIO_Pin_4) == 0)
		{
			s++;
			//连续两次检测到按键按下，跳转到IAP
			if(s >= 2)
			{
				jumpToIap();
			}
		}
		else
		{
			s = 0;
		}
		DelayMs(100);
    }
}
