/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : SPI0演示 Master/Slave 模式数据收发
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"

__attribute__((aligned(4))) UINT8 spiBuff[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6};
__attribute__((aligned(4))) UINT8 spiBuffrev[16];

void DebugInit(void)
{
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
}

int main()
{
    UINT8 i;

    SetSysClock(CLK_SOURCE_PLL_60MHz);

    /* 配置串口调试 */
    DebugInit();
    PRINT("Start @ChipID=%02X\n", R8_CHIP_ID);

#if 1
    /* 主机模式 */
    PRINT("1.spi0 mul master mode send data ...\n");
    DelayMs(100);

  #if 1
    /* SPI 0 */
    GPIOA_SetBits(GPIO_Pin_12);
    GPIOA_ModeCfg(GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14, GPIO_ModeOut_PP_5mA);
    SPI0_MasterDefInit();

    // 单字节发送
    GPIOA_ResetBits(GPIO_Pin_12);
    SPI0_MasterSendByte(0x55);
    GPIOA_SetBits(GPIO_Pin_12);
    DelayMs(1);
    GPIOA_ResetBits(GPIO_Pin_12);
    i = SPI0_MasterRecvByte();
    GPIOA_SetBits(GPIO_Pin_12);
    DelayMs(2);

    // FIFO 连续发送
    GPIOA_ResetBits(GPIO_Pin_12);
    SPI0_MasterTrans(spiBuff, 12);
    GPIOA_SetBits(GPIO_Pin_12);
    DelayMs(2);
    GPIOA_ResetBits(GPIO_Pin_12);
    SPI0_MasterRecv(spiBuffrev, 12);
    GPIOA_SetBits(GPIO_Pin_12);
    DelayMs(2);

    // DMA 连续发送
    GPIOA_ResetBits(GPIO_Pin_12);
    SPI0_MasterDMATrans(spiBuff, 12);
    GPIOA_SetBits(GPIO_Pin_12);
    DelayMs(2);
    GPIOA_ResetBits(GPIO_Pin_12);
    SPI0_MasterDMARecv(spiBuffrev, 12);
    GPIOA_SetBits(GPIO_Pin_12);

  #else
    /* SPI 1 仅CH583支持*/
    GPIOA_SetBits(GPIO_Pin_3);
    GPIOA_ModeCfg(GPIO_Pin_3 | GPIO_Pin_0 | GPIO_Pin_1, GPIO_ModeOut_PP_5mA);
    SPI1_MasterDefInit();

    // 单字节发送
    GPIOA_ResetBits(GPIO_Pin_3);
    SPI1_MasterSendByte(0x55);
    GPIOA_SetBits(GPIO_Pin_3);
    DelayMs(1);
    GPIOA_ResetBits(GPIO_Pin_3);
    i = SPI1_MasterRecvByte();
    GPIOA_SetBits(GPIO_Pin_3);
    DelayMs(2);

    // FIFO 连续发送
    GPIOA_ResetBits(GPIO_Pin_3);
    SPI1_MasterTrans(spiBuff, 12);
    GPIOA_SetBits(GPIO_Pin_3);
    DelayMs(2);
    GPIOA_ResetBits(GPIO_Pin_3);
    SPI1_MasterRecv(spiBuffrev, 12);
    GPIOA_SetBits(GPIO_Pin_3);
    DelayMs(2);

    // FIFO 连续发送
    GPIOA_ResetBits(GPIO_Pin_3);
    SPI1_MasterTrans(spiBuff, 12);
    GPIOA_SetBits(GPIO_Pin_3);
    DelayMs(2);
    GPIOA_ResetBits(GPIO_Pin_3);
    SPI1_MasterRecv(spiBuffrev, 12);
    GPIOA_SetBits(GPIO_Pin_3);

  #endif
    PRINT("END ...\n");
    while(1);
#endif

#if 0
    /* 设备模式 */
    PRINT("1.spi0 mul slave mode \n");
    GPIOA_ModeCfg(GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15, GPIO_ModeIN_PU);
    SPI0_SlaveInit();
    i = SPI0_SlaveRecvByte();
    SPI0_SlaveSendByte(~i);

    SPI0_SlaveRecv(spiBuffrev, 12);
    SPI0_SlaveTrans(spiBuffrev, 12);
    SPI0_SlaveDMARecv(spiBuffrev, 12);
    SPI0_SlaveDMATrans(spiBuffrev, 12);
    PRINT("END ...\n");

    while(1);
#endif

    while(1);
}
