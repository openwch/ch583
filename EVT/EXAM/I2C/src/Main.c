/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : I2C¹¦ÄÜÑÝÊ¾
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"

/* I2C Mode Definition */
#define HOST_MODE     0
#define SLAVE_MODE    1

/* I2C Communication Mode Selection */
#define I2C_MODE      HOST_MODE
//#define I2C_MODE   SLAVE_MODE

/* Global define */
#define Size          7
#define RxAdderss     0x53
#define TxAdderss     0x52

/* Global Variable */
uint8_t TxData[Size] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8_t RxData[Size];

int main()
{
    uint8_t i = 0;
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
    GPIOB_ModeCfg(GPIO_Pin_13 | GPIO_Pin_12, GPIO_ModeIN_PU);

#if(I2C_MODE == HOST_MODE)
    printf("IIC Host mode\r\n");
    I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, TxAdderss);
    while(I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(ENABLE);

    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(RxAdderss, I2C_Direction_Transmitter);

    while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    while(i < 6)
    {
        if(I2C_GetFlagStatus(I2C_FLAG_TXE) != RESET)
        {
            I2C_SendData(TxData[i]);
            i++;
        }
    }

    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTOP(ENABLE);

#elif(I2C_MODE == SLAVE_MODE)
    printf("IIC Slave mode\r\n");
    I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, RxAdderss);

    while(!I2C_CheckEvent(I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED));

    while(i < 6)
    {
        if(I2C_GetFlagStatus(I2C_FLAG_RXNE) != RESET)
        {
            RxData[i] = I2C_ReceiveData();
            i++;
        }
    }

    printf("RxData:\r\n");
    for(i = 0; i < 6; i++)
    {
        printf("%02x\r\n", RxData[i]);
    }

#endif

    while(1);
}
