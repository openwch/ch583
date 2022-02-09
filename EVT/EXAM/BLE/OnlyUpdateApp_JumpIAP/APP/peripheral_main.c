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

#define IMAGE_ADDR    0xC000

__attribute__((aligned(4))) int Image_Flag __attribute__((section(".ImageFlag"))) =
    (0x6F << 0) |
    ((IMAGE_ADDR >> (15 - 7) & (0xe0)) << 8) |
    (((IMAGE_ADDR << (7 - 3) & (0xe0)) | (IMAGE_ADDR >> (11 - 4) & (0x10)) | (IMAGE_ADDR >> (19 - 3) & (0x0f))) << 16) |
    (((((IMAGE_ADDR >> (20 - 7)) & (0x80)) | ((IMAGE_ADDR >> (10 - 6)) & (0x3f)))) << 24);

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
