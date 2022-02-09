/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : 跳转指令
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/

#include "OTA.h"

#define IMAGE_ADDR    IMAGE_IAP_START_ADD

// 20 10:1 11 19:12
__attribute__((aligned(4))) int Image_Flag __attribute__((section(".ImageFlag"))) =
    (0x6F << 0) |
    (((IMAGE_ADDR >> (12 - 4)) & 0xF0) << 8) |
    ((((IMAGE_ADDR >> 16) & 0x0F) | ((IMAGE_ADDR >> (11 - 4)) & 0x10) | ((IMAGE_ADDR << 5) & 0xE0)) << 16) |
    ((((IMAGE_ADDR >> 4) & 0x7F) | ((IMAGE_ADDR >> (20 - 7)) & 0x80)) << 24);
/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
}

/******************************** endfile @ main ******************************/
