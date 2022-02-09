/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/03/09
 * Description        : adc采样示例，包括温度检测、单通道检测、差分通道检测、TouchKey检测、中断方式采样。
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"

uint16_t abcBuff[40];

volatile uint8_t adclen;
volatile uint8_t DMA_end = 0;

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
    uint8_t      i;
    signed short RoughCalib_Value = 0; // ADC粗调偏差值

    SetSysClock(CLK_SOURCE_PLL_60MHz);

    /* 配置串口调试 */
    DebugInit();
    PRINT("Start @ChipID=%02X\n", R8_CHIP_ID);
    /* 温度采样并输出， 包含数据校准 */
    PRINT("\n1.Temperature sampling...\n");
    ADC_InterTSSampInit();
    RoughCalib_Value = ADC_DataCalib_Rough(); // 用于计算ADC内部偏差，记录到变量 RoughCalib_Value中，注意这个变量需要定义为有符号变量

    for(i = 0; i < 20; i++)
    {
        abcBuff[i] = ADC_ExcutSingleConver() + RoughCalib_Value; // 连续采样20次
    }
    for(i = 0; i < 20; i++)
    {
        PRINT("%d \n", abcBuff[i]);
    }

    /* 单通道采样：选择adc通道0做采样，对应 PA4引脚， 带数据校准功能 */
    PRINT("\n2.Single channel sampling...\n");
    GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_Floating);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);

    RoughCalib_Value = ADC_DataCalib_Rough(); // 用于计算ADC内部偏差，记录到全局变量 RoughCalib_Value中
    PRINT("RoughCalib_Value =%d \n", RoughCalib_Value);

    ADC_ChannelCfg(0);

    for(i = 0; i < 20; i++)
    {
        abcBuff[i] = ADC_ExcutSingleConver() + RoughCalib_Value; // 连续采样20次
    }
    for(i = 0; i < 20; i++)
    {
        PRINT("%d \n", abcBuff[i]); // 注意：由于ADC内部偏差的存在，当采样电压在所选增益范围极限附近的时候，可能会出现数据溢出的现象
    }

    /* DMA单通道采样：选择adc通道0做采样，对应 PA4引脚 */
    GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_Floating);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(0);
    ADC_AutoConverCycle(192); // 采样周期为 (256-192)*16个系统时钟
    ADC_DMACfg(ENABLE, (uint16_t)(uint32_t)&abcBuff[0], (uint16_t)(uint32_t)&abcBuff[40], ADC_Mode_Single);
    PFIC_EnableIRQ(ADC_IRQn);
    ADC_StartDMA();
    while(!DMA_end);
    DMA_end = 0;
    PRINT("ADC DMA end \n");
    for(i = 0; i < 40; i++)
    {
        PRINT("%d \n", abcBuff[i]);
    }

    /* 差分通道采样：选择adc通道0做采样，对应 PA4(AIN0)、PA12(AIN2) */
    PRINT("\n3.Diff channel sampling...\n");
    GPIOA_ModeCfg(GPIO_Pin_4 | GPIO_Pin_12, GPIO_ModeIN_Floating);
    ADC_ExtDiffChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(0);
    for(i = 0; i < 20; i++)
    {
        abcBuff[i] = ADC_ExcutSingleConver(); // 连续采样20次
    }
    for(i = 0; i < 20; i++)
    {
        PRINT("%d \n", abcBuff[i]);
    }

    /* TouchKey采样：选择adc通道 2 做采样，对应 PA12 */
    PRINT("\n4.TouchKey sampling...\n");
    GPIOA_ModeCfg(GPIO_Pin_12, GPIO_ModeIN_Floating);
    TouchKey_ChSampInit();
    ADC_ChannelCfg(2);

    for(i = 0; i < 20; i++)
    {
        abcBuff[i] = TouchKey_ExcutSingleConver(0x10, 0); // 连续采样20次
    }
    for(i = 0; i < 20; i++)
    {
        PRINT("%d \n", abcBuff[i]);
    }

    /* 单通道采样：中断方式,选择adc通道1做采样，对应 PA5引脚， 不带数据校准功能 */
    PRINT("\n5.Single channel sampling in interrupt mode...\n");
    GPIOA_ModeCfg(GPIO_Pin_5, GPIO_ModeIN_Floating);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(1);
    adclen = 0;
    ADC_ClearITFlag();
    PFIC_EnableIRQ(ADC_IRQn);

    ADC_StartUp();
    while(adclen < 20);
    PFIC_DisableIRQ(ADC_IRQn);
    for(i = 0; i < 20; i++)
    {
        PRINT("%d \n", abcBuff[i]);
    }

    while(1);
}

/*********************************************************************
 * @fn      ADC_IRQHandler
 *
 * @brief   ADC中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void ADC_IRQHandler(void) //adc中断服务程序
{
    if(ADC_GetDMAStatus())
    {
        ADC_ClearDMAFlag();
        ADC_StopDMA();
        R16_ADC_DMA_BEG = (uint16_t)(uint32_t)&abcBuff[0];
        DMA_end = 1;
    }
    if(ADC_GetITStatus())
    {
        ADC_ClearITFlag();
        if(adclen < 20)
        {
            abcBuff[adclen] = ADC_ReadConverValue();
            ADC_StartUp(); // 作用清除中断标志并开启新一轮采样
        }
        adclen++;
    }
}
