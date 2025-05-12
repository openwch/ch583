/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : ����ӻ�Ӧ��������������ϵͳ��ʼ��
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* ͷ�ļ����� */
#include "CONFIG.h"
#include "HAL.h"
#include "peripheral.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
u8C MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif



volatile uint8_t switch_sta = FALSE;
uint8_t  key_flag = 0;

/*******************************************************************************
 * Function Name  : Main_Circulation
 * Description    : ��ѭ��
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    while(1)
    {
        TMOS_SystemProcess();
    }
}

/*******************************************************************************
 * Function Name  : main
 * Description    : ������
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
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
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    PRINT("%s\n", VER_LIB);

    GPIOB_ModeCfg(LED_LIGHT_PIN | LED_LINK_PIN, GPIO_ModeOut_PP_5mA);
    GPIOB_SetBits(LED_LIGHT_PIN | LED_LINK_PIN);

    GPIOB_ModeCfg(KEY_SWITCH_PIN, GPIO_ModeIN_PU);
    GPIOB_ITModeCfg(KEY_SWITCH_PIN, GPIO_ITMode_FallEdge);
    PFIC_EnableIRQ(GPIO_B_IRQn);

    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();
    Main_Circulation();
}

/*********************************************************************
 * @fn      GPIOA_IRQHandler
 *
 * @brief   GPIOA�жϺ���
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void GPIOB_IRQHandler(void)
{
    if(GPIOB_ReadITFlagBit(KEY_SWITCH_PIN))
    {
            switch_sta = !switch_sta;
            tmos_start_task(Peripheral_TaskID, IOCHUB_SWITCH_CHANGE_EVT, 2);
            GPIOB_ClearITFlagBit(KEY_SWITCH_PIN);
    }
}

/******************************** endfile @ main ******************************/
