/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_flag.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/03/15
 * Description        : USB IAP APP例程
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "app_flag.h"

/*********************************************************************
 * @fn      SwitchImageFlag
 *
 * @brief   切换dataflash里的Flag,若想切换到IAP，则将该flag设置为ImageFlag_USER_PROGRAM_CALL_IAP，跳转到0位置即可
 *
 * @param   new_flag    - 切换的Flag
 *
 * @return  none
 */
void SwitchImageFlag(uint8_t new_flag)
{
    UINT16 i;
    UINT32 ver_flag;
    __attribute__((aligned(4)))   IAPDataFlashInfo_t imgFlag;
    /* 读取第一块 */
    EEPROM_READ(IAP_FLAG_DATAFLASH_ADD, (PUINT32) &imgFlag, 4);
    if (imgFlag.ImageFlag != new_flag)
    {
        /* 擦除第一块 */
        EEPROM_ERASE(IAP_FLAG_DATAFLASH_ADD, EEPROM_PAGE_SIZE);

        /* 更新Image信息 */
        imgFlag.ImageFlag = new_flag;

        /* 编程DataFlash */
        EEPROM_WRITE(IAP_FLAG_DATAFLASH_ADD, (PUINT32) &imgFlag, 4);
    }
}

/*********************************************************************
 * @fn      jumpToIap
 *
 * @brief   跳转到IAP
 *
 * @return  none
 */
void jumpToIap(void)
{
    uint32_t irq_status;
    SwitchImageFlag(FLAG_USER_CALL_IAP);//如果IAP采用按键检测，将该句话注释掉
    SYS_DisableAllIrq(&irq_status);
    SYS_ResetExecute();
}
