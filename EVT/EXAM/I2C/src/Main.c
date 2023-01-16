/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : I2C¹¦ÄÜÑÝÊ¾
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"
#include "app_i2c.h"

/* I2C Mode Definition */
#define HOST_MODE     0
#define SLAVE_MODE    1

/* I2C Communication Mode Selection */
#define I2C_MODE      HOST_MODE
//#define I2C_MODE      SLAVE_MODE

/* Global define */
#define SIZE            7
#define MASTER_ADDR     0x42
#define SLAVE_ADDR      0x52

/* Global Variable */
uint8_t TxData[SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
uint8_t RxData[SIZE];

#if (I2C_MODE == SLAVE_MODE)
static void i2c_on_slave_transmit_user(uint8_t *data, uint8_t *len)
{
    *len = sizeof(TxData);
    memcpy(data, TxData, sizeof(TxData));
}

static void i2c_on_slave_receive_user(uint8_t *data, uint8_t len)
{
    PRINT("I2C slave receiver callback: received (");
    for(int i = 0; i < len; i++) {
        PRINT(" %#x", data[i]);
    }
    PRINT(" )\n");
}
#endif

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
    PRINT("IIC Host mode\r\n");
    I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, MASTER_ADDR);
    while(I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(ENABLE);

    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(SLAVE_ADDR, I2C_Direction_Transmitter);

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

    /* I2C INT HOST */
    DelayMs(100);
    i2c_app_init(MASTER_ADDR);

    int ret;
    ret = i2c_write_to(SLAVE_ADDR >> 1, TxData, sizeof(TxData), true, true);
    PRINT("write to %#x %s\n", SLAVE_ADDR >> 1, ret ? "failed" : "success");

    ret = i2c_read_from(SLAVE_ADDR >> 1, RxData, 4, true, 1000);
    PRINT("read %d byte(s) from %#x (", ret, SLAVE_ADDR >> 1);

    if (ret >= 0) {
        for(int i = 0; i < ret; i++) {
            PRINT(" %#x", RxData[i]);
        }
    }
    PRINT(" )\n");

#elif(I2C_MODE == SLAVE_MODE)
    PRINT("IIC Slave mode\r\n");
    I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, SLAVE_ADDR);

    while(!I2C_CheckEvent(I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED));

    while(i < 6)
    {
        if(I2C_GetFlagStatus(I2C_FLAG_RXNE) != RESET)
        {
            RxData[i] = I2C_ReceiveData();
            i++;
        }
    }

    PRINT("RxData ( ");
    for(i = 0; i < 6; i++)
    {
        PRINT("%#x ", RxData[i]);
    }
    PRINT(")\n");

    struct i2c_slave_cb slave_user = {
        .on_receive = i2c_on_slave_receive_user,
        .on_transmit = i2c_on_slave_transmit_user,
    };

    i2c_app_init(SLAVE_ADDR);
    i2c_slave_cb_register(&slave_user);

#endif

    while(1);
}
