/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_sec.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/09/17
 * Description        : lwns消息加密
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_sec.h"

static uint8_t lwns_sec_key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}; //用户自行更改为自己的秘钥，或者改为可以从主机获取，并存储在eeprom中

/*********************************************************************
 * @fn      lwns_msg_encrypt
 *
 * @brief   lwns消息加密
 *
 * @param   src     -   待加密的数据缓冲头指针.
 * @param   to      -   待存储加密数据的数据缓存区头指针.
 * @param   mlen    -   待加密的数据长度.
 *
 * @return  加密后的数据长度.
 */
int lwns_msg_encrypt(uint8_t *src, uint8_t *to, uint8_t mlen)
{
    uint16_t i = 0;
    uint8_t  esrc[16];
    while(1)
    {
        if((mlen - i) < 16)
        {
            tmos_memcpy(esrc, src + i, (mlen - i)); //扩充到16字节，其他为0
            LL_Encrypt(lwns_sec_key, esrc, to + i);
        }
        else
        {
            LL_Encrypt(lwns_sec_key, src + i, to + i);
        }
        i += 16;
        if(i >= mlen)
        {
            break;
        }
    }
    return i; //返回加密后数据长度
}

/*********************************************************************
 * @fn      lwns_msg_decrypt
 *
 * @brief   lwns消息解密
 *
 * @param   src     -   待解密的数据缓冲头指针.
 * @param   to      -   待存储解密数据的数据缓存区头指针.
 * @param   mlen    -   待解密的数据长度，必须为16的倍数.
 *
 * @return  解密后的数据长度.
 */
int lwns_msg_decrypt(uint8_t *src, uint8_t *to, uint8_t mlen)
{
    unsigned short i = 0;
    while(1)
    {
        LL_Decrypt(lwns_sec_key, src + i, to + i);
        i += 16;
        if(i >= mlen)
        {
            break;
        }
    }
    return i;
}
