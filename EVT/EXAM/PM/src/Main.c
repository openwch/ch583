/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 系统睡眠模式并唤醒演示：GPIOA_5作为唤醒源，共4种睡眠等级
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*
 注意：切换到HSE时钟源，所需等待稳定时间和选择的外置晶体参数有关，选择一款新的晶体最好阅读厂家提供的晶体及其
 负载电容参数值。通过配置R8_XT32M_TUNE寄存器，可以配置不同的负载电容和偏置电流，调整晶体稳定时间。
 */

#include "CH58x_common.h"

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
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);

    /* 配置串口调试 */
    DebugInit();
    PRINT("Start @ChipID=%02x\n", R8_CHIP_ID);
    DelayMs(200);

#if 1
    /* 配置唤醒源为 GPIO - PA5 */
    GPIOA_ModeCfg(GPIO_Pin_5, GPIO_ModeIN_PU);
    GPIOA_ITModeCfg(GPIO_Pin_5, GPIO_ITMode_FallEdge); // 下降沿唤醒
    PFIC_EnableIRQ(GPIO_A_IRQn);
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);
#endif

#if 1
    PRINT("IDLE mode sleep \n");
    DelayMs(1);
    LowPower_Idle();
    PRINT("wake.. \n");
    DelayMs(500);
#endif

#if 1
    PRINT("Halt mode sleep \n");
    DelayMs(2);
    LowPower_Halt();
    HSECFG_Current(HSE_RCur_100); // 降为额定电流(低功耗函数中提升了HSE偏置电流)
    DelayMs(2);
    PRINT("wake.. \n");
    DelayMs(500);
#endif

#if 1
    PRINT("sleep mode sleep \n");
    DelayMs(2);
    // 注意当主频为80M时，Sleep睡眠唤醒中断不可调用flash内代码。
    LowPower_Sleep(RB_PWR_RAM30K | RB_PWR_RAM2K); //只保留30+2K SRAM 供电
    HSECFG_Current(HSE_RCur_100);                 // 降为额定电流(低功耗函数中提升了HSE偏置电流)
    DelayMs(5);
    PRINT("wake.. \n");
    DelayMs(500);
#endif

#if 1
    PRINT("shut down mode sleep \n");
    DelayMs(2);
    LowPower_Shutdown(0); //全部断电，唤醒后复位
    /*
     此模式唤醒后会执行复位，所以下面代码不会运行，
     注意要确保系统睡下去再唤醒才是唤醒复位，否则有可能变成IDLE等级唤醒
     */
    HSECFG_Current(HSE_RCur_100); // 降为额定电流(低功耗函数中提升了HSE偏置电流)
    PRINT("wake.. \n");
    DelayMs(500);
#endif

    while(1)
        ;
}

/*********************************************************************
 * @fn      GPIOA_IRQHandler
 *
 * @brief   GPIOA中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    GPIOA_ClearITFlagBit(GPIO_Pin_6 | GPIO_Pin_5);
}
