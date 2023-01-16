/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : FALSH读写例程
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"
#include "iap.h"

IAPDataFlashInfo_t p_image_flash;

/*********************************************************************
 * @fn      mySetSysClock
 *
 * @brief   配置系统运行时钟60Mhz, 0x48
 *
 * @param   none
 *
 * @return  none
 */
__HIGH_CODE
void mySetSysClock()
{
    uint32_t i;
    sys_safe_access_enable();
    R8_PLL_CONFIG &= ~(1 << 5); //
    sys_safe_access_disable();
    // PLL div
    if(!(R8_HFCK_PWR_CTRL & RB_CLK_PLL_PON))
    {
        sys_safe_access_enable();
        R8_HFCK_PWR_CTRL |= RB_CLK_PLL_PON; // PLL power on
        for(i = 0; i < 2000; i++)
        {
            __nop();
            __nop();
        }
    }
    sys_safe_access_enable();
    R16_CLK_SYS_CFG = (1 << 6) | (CLK_SOURCE_PLL_60MHz & 0x1f);
    __nop();
    __nop();
    __nop();
    __nop();
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_FLASH_CFG = 0X52;
    sys_safe_access_disable();
    //更改FLASH clk的驱动能力
    sys_safe_access_enable();
    R8_PLL_CONFIG |= 1 << 7;
    sys_safe_access_disable();
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
    uint16_t i;
    uint8_t  s;

    mySetSysClock();

#if USE_EEPROM_FLAG
    EEPROM_READ(IAP_FLAG_DATAFLASH_ADD, &p_image_flash, 4);
    if ((p_image_flash.ImageFlag != FLAG_USER_CALL_IAP))
    {
        jumpApp();
    }
#else
    //初始化引脚为上拉输入。为了减小程序大小，采用寄存器编写。
    R32_PB_PD_DRV &= ~GPIO_Pin_4;
    R32_PB_PU |= GPIO_Pin_4;
    R32_PB_DIR &= ~GPIO_Pin_4;
    //
    DelayMs(10);
    if (GPIOB_ReadPortPin(GPIO_Pin_4))
    {
        DelayMs(5);
        if (GPIOB_ReadPortPin(GPIO_Pin_4))
        {
            //启动前判断是否进入iap
            jumpApp();
        }
    }
#endif

    /* uart初始化，根据需要更改成自己的uart */
    GPIOA_SetBits( bTXD1 );

    /* 为了节约代码空间，初始化尽量都使用寄存器 */
    R32_PA_PD_DRV &= ((~bTXD1) & (~bRXD1));
    /* GPIOA_ModeCfg( bTXD1, GPIO_ModeOut_PP_5mA ); */
    //R32_PA_PD_DRV &= ~bTXD1;
    R32_PA_DIR    |= bTXD1;

    /* GPIOA_ModeCfg(bRXD1, GPIO_ModeIN_PU); */
    //R32_PA_PD_DRV &= ~bRXD1;
    R32_PA_PU     |= bRXD1;
    R32_PA_DIR    &= ~bRXD1;

    UART1_BaudRateCfg( 115200 );
    R8_UART1_FCR = (2<<6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;   // FIFO打开，触发点4字节
    R8_UART1_LCR = RB_LCR_WORD_SZ;
    R8_UART1_IER = RB_IER_TXD_EN;
    R8_UART1_DIV = 1;

    Main_Circulation();
}
