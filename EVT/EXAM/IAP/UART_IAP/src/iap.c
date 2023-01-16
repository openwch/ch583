/********************************** (C) COPYRIGHT *******************************
 * File Name          : iap.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/03/15
 * Description        : UART IAP例程
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "iap.h"

/* 更新权限，必须先擦除后，且擦除地址需要和APP对应地址一致才可赋予更新权限 */
uint8_t g_update_permition = 0;

uint8_t iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1, all_data_rec_cnt = 0, part_rec_cnt = 0, uart_rec_sign = 0;

iap_cmd_t iap_rec_data;

__attribute__((aligned(4)))                    uint8_t g_write_buf[256 + 64];

uint16_t g_buf_write_ptr = 0;

uint32_t g_flash_write_ptr = 0;

uint32_t g_tcnt;

uint32_t g_addr;

__attribute__((aligned(4)))   uint8_t iap_rsp_data[6] = {IAP_DATA_SOP1, IAP_DATA_SOP2, 0, 0, IAP_DATA_EOP1, IAP_DATA_EOP2};

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   IAP主循环,程序放ram中运行，提升速度.
 *
 * @param   None.
 *
 * @return  None.
 */
__attribute__((section(".highcode")))
void Main_Circulation()
{
    while (1)
    {
        /* 采用查询模式，不使用中断，减少代码占用flash */
        if (R8_UART1_RFC)
        {
            /* 串口有数据，置位串口处于接收过程标志位 */
            uart_rec_sign = 1;
            /* 串口有数据，清空超时时间 */
            g_tcnt = 0;
            /* 接收计数加一，存储到buf中 */
            all_data_rec_cnt++;
            /* buf不可越界 */
            if (all_data_rec_cnt >= sizeof(iap_rec_data))
            {
                all_data_rec_cnt = 0;
            }
            /* 读取串口寄存器数据，直接赋值到buf中，后续判断无需继续赋值，可以节约程序flash占用 */
            iap_rec_data.other.buf[all_data_rec_cnt] = R8_UART1_RBR;
            /*
             * 这里为了节约代码占用空间和代码运行速度，做了相应的代码裁剪，
             * 否则应该根据命令码，将校验值和包尾信息存到相应的结构体的checksum和eop成员变量中。
             * 本程序的处理方法为直接按顺序存储到buf中，
             * 所以在数据长度不为满包数据长度时，通过结构体访问成员checksum和eop变量会不正确。
             * 用户自行修改时需注意。
             * 相比于每次都根据包长判断来说，每次都节约了对命令的判断时间、代码flash空间。
             */
            switch (iap_rec_data_state) /* 根据接收状态进行数据的读取判断和存储 */
            {
            /* 状态处于等待包头1时，判断接收的字节是否为IAP_DATA_SOP1 */
            case IAP_DATA_REC_STATE_WAIT_SOP1:
                if (iap_rec_data.other.buf[all_data_rec_cnt] == IAP_DATA_SOP1)
                {
                    /* 只有第一个字节时，不一定存储到buf正确的位置，重新存储 */
                    iap_rec_data.other.buf[0] = iap_rec_data.other.buf[all_data_rec_cnt];
                    /* 接收计数初始化 */
                    all_data_rec_cnt = 0;
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP2;
                }
                break;
            /* 状态处于等待包头2时，判断接收的字节是否为IAP_DATA_SOP2 */
            case IAP_DATA_REC_STATE_WAIT_SOP2:
                if (iap_rec_data.other.buf[all_data_rec_cnt] == IAP_DATA_SOP2)
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_CMD;
                }
                else
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                }
                break;
            /* 状态处于等待命令码时，判断接收的字节是否为合法的cmd */
            case IAP_DATA_REC_STATE_WAIT_CMD:
                if ((iap_rec_data.other.buf[all_data_rec_cnt] < CMD_IAP_PROM) || (iap_rec_data.other.buf[all_data_rec_cnt] > CMD_IAP_END))
                {
                    /* error 没有这个cmd */
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                }
                else
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_LEN;
                }
                break;
            /* 状态处于等待数据长度时，判断接收的字节是否为合法的长度 */
            case IAP_DATA_REC_STATE_WAIT_LEN:
                if (iap_rec_data.other.buf[all_data_rec_cnt] <= IAP_LEN)
                {
                    /* 清空部分结构体变量接收字节数计数 */
                    part_rec_cnt = 0;
                    if ((iap_rec_data.other.buf[2] == CMD_IAP_ERASE) || (iap_rec_data.other.buf[2] == CMD_IAP_VERIFY))
                    {
                        iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_ADDR;
                    }
                    else
                    {
                        /* 判断数据长度是否为0，为0则直接接收校验和*/
                        if (iap_rec_data.other.buf[3] > 0)
                        {
                            iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_DATA;
                        }
                        else
                        {
                            iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_CHECKNUM;
                        }
                    }
                }
                else
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                }
                break;
            /* 状态处于等待地址时 */
            case IAP_DATA_REC_STATE_WAIT_ADDR:
                part_rec_cnt++;
                /* 地址为4字节，接收到4个后跳转接收下一个状态 */
                if (part_rec_cnt >= 4)
                {
                    /* 部分结构体变量接收字节数计数 */
                    part_rec_cnt = 0;
                    if (iap_rec_data.other.buf[3] > 0)
                    {
                        iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_DATA;
                    }
                    else
                    {
                        iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_CHECKNUM;
                    }
                }
                break;
            /* 状态处于等待数据时 */
            case IAP_DATA_REC_STATE_WAIT_DATA:
                part_rec_cnt++;
                if (part_rec_cnt >= iap_rec_data.other.buf[3])
                {
                    /* 判断数据是否接收完成*/
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_CHECKNUM;
                    /* 部分结构体变量接收字节数计数 */
                    part_rec_cnt = 0;
                }
                break;
            /* 状态处于等待校验时 */
            case IAP_DATA_REC_STATE_WAIT_CHECKNUM:
                part_rec_cnt++;
                if (part_rec_cnt >= 2)
                {
                    /* 判断校验是否接收完成，校验为2字节和校验*/
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_EOP1;
                }
                break;
            /* 状态处于等待包尾1时，判断接收的字节是否为IAP_DATA_EOP1 */
            case IAP_DATA_REC_STATE_WAIT_EOP1:
                if (iap_rec_data.other.buf[all_data_rec_cnt] == IAP_DATA_EOP1)
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_EOP2;
                }
                else
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                }
                break;
            /* 状态处于等待包尾2时，判断接收的字节是否为IAP_DATA_EOP2 */
            case IAP_DATA_REC_STATE_WAIT_EOP2:
                if (iap_rec_data.other.buf[all_data_rec_cnt] == IAP_DATA_EOP2)
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_OK;
                }
                else
                {
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                }
                break;
            default:
                /* 一般不可能出现这种情况 */
                break;
            }

            if (iap_rec_data_state == IAP_DATA_REC_STATE_OK)
            {
                /* 计算校验和 */
                uint16_t   check_num = 0, check_num_rec;
                /* 校验和计算所用 */
                uint16_t   check_num_i;
                /* 上报错误码清为默认无错误状态 */
                iap_rsp_data[2] = 0x00;
                iap_rsp_data[3] = 0x00;
                /* 恢复默认的状态 */
                iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                /* 解析到一个完整的数据包，释放空闲计时 */
                uart_rec_sign = 0;
                g_tcnt = 0;
                /* 计算校验和验算 */
                for (check_num_i = 2; check_num_i < all_data_rec_cnt - 3; check_num_i++)
                {
                    check_num += iap_rec_data.other.buf[check_num_i];
                }
                check_num_rec = iap_rec_data.other.buf[check_num_i] | (iap_rec_data.other.buf[check_num_i + 1] << 8);
                /* 数据包校验和通过 */
                if (check_num_rec == check_num)
                {
                    /* 判断命令 */
                    switch (iap_rec_data.other.buf[2])
                    {
                    /* 写入命令 */
                    case CMD_IAP_PROM:
                        /* 判断是否先擦除过芯片 */
                        if (g_update_permition == 1)
                        {
                            if (iap_rec_data.program.len == 0)
                            {
                                /* 最后一次为空包，宣布为最后一个包 */
                                if (g_buf_write_ptr != 0)
                                {
                                    if (FLASH_ROM_WRITE(g_flash_write_ptr, (PUINT32)g_write_buf, g_buf_write_ptr))
                                    {
                                        iap_rsp_data[2] = 0xfe;
                                        iap_rsp_data[3] = IAP_ERR_WRITE_FAIL;
                                    }
                                    g_buf_write_ptr = 0;
                                }
                            }
                            else
                            {
                                my_memcpy(g_write_buf + g_buf_write_ptr, iap_rec_data.program.data, iap_rec_data.program.len);
                                g_buf_write_ptr += iap_rec_data.program.len;
                                if (g_buf_write_ptr >= 256)
                                {
                                    /* 满256字节写一次 */
                                    if (FLASH_ROM_WRITE(g_flash_write_ptr, (PUINT32)g_write_buf, 256))
                                    {
                                        iap_rsp_data[2] = 0xfe;
                                        iap_rsp_data[3] = IAP_ERR_WRITE_FAIL;
                                        break;
                                    }
                                    /* 移动指针 */
                                    g_flash_write_ptr += 256;
                                    /* 重新计算超出的长度，将超出的数据拷贝到数组首部 */
                                    g_buf_write_ptr = g_buf_write_ptr - 256;
                                    my_memcpy(g_write_buf, g_write_buf + 256, g_buf_write_ptr);
                                }
                            }
                        }
                        else
                        {
                            /* 没有擦除步骤，不可更新，报错 */
                            iap_rsp_data[2] = 0xfe;
                            iap_rsp_data[3] = IAP_ERR_PROG_NO_ERASE;
                        }
                        break;
                    /* 擦除命令 */
                    case CMD_IAP_ERASE:
                        if (iap_rec_data.erase.addr == APP_CODE_START_ADDR)
                        {
                            /* 如果有特殊需求，可以修改擦除的长度，将特殊需求的那一部分flash排除在外 */
                            if (FLASH_ROM_ERASE(APP_CODE_START_ADDR, APP_CODE_END_ADDR - APP_CODE_START_ADDR) == 0)
                            {
                                /* 起始地址正确，赋予更新权限，否则认为失败 */
                                g_update_permition = 1;
                                /* 计数清零，flash写指针重置 */
                                g_buf_write_ptr = 0;
                                g_flash_write_ptr = APP_CODE_START_ADDR;
                            }
                            else
                            {
                                /* 擦除失败 */
                                iap_rsp_data[2] = 0xfe;
                                iap_rsp_data[3] = IAP_ERR_ERASE_FAIL;
                            }
                        }
                        else
                        {
                            /* 擦除地址错误 */
                            iap_rsp_data[2] = 0xfe;
                            iap_rsp_data[3] = IAP_ERR_ADDR;
                        }
                        break;
                    /* 校验命令 */
                    case CMD_IAP_VERIFY:
                        if (((iap_rec_data.verify.addr % 4) == 0) && (iap_rec_data.verify.addr >= APP_CODE_START_ADDR) && (iap_rec_data.verify.addr < APP_CODE_END_ADDR))
                        {
                            my_memcpy(g_write_buf, iap_rec_data.verify.data, iap_rec_data.verify.len);
                            if (FLASH_ROM_VERIFY(iap_rec_data.verify.addr, g_write_buf, iap_rec_data.verify.len))
                            {
                                /* 校验失败，报错 */
                                iap_rsp_data[2] = 0xfe;
                                iap_rsp_data[3] = IAP_ERR_VERIFY;
                            }
                        }
                        else
                        {
                            /* 校验地址不对 */
                            iap_rsp_data[2] = 0xfe;
                            iap_rsp_data[3] = IAP_ERR_ADDR;
                        }
                        break;
                    /* 结束跳转命令 */
                    case CMD_IAP_END:
                        jumpApp();
                        break;
                    default:
                        /* 接收时已判断命令不可能为其他值，所以不会出现该情况 */
                        iap_rsp_data[2] = 0xfe;
                        iap_rsp_data[3] = IAP_ERR_UNKNOWN;
                        break;
                    }
                    if (iap_rsp_data[2] != 0)
                    {
                        /* 校验和通过，由于其他错误出错后，清空更新权限，想要更新，按步骤重新开始 */
                        g_update_permition = 0;
                    }
                }
                else
                {
                    /* 数据包校验和失败，主机可以选择重发数据包，无影响 */
                    iap_rsp_data[2] = 0xfe;
                    iap_rsp_data[3] = IAP_ERR_CHECK;
                }
                /* 每次数据包处理完，清空缓存其他的数据，防止有些串口模块偶然会多发出一两个字符信号，保持一包一回复 */
                while (R8_UART1_RFC)
                {
                    iap_rec_data.other.buf[all_data_rec_cnt] = R8_UART1_RBR;
                }
                /* 回复数据 */
                UART1_SendString(iap_rsp_data, sizeof(iap_rsp_data));
            }
        }
        else
        {
            /* 延迟115200波特率下大概四分之一个字节的时间，降低读取寄存器频率和时间，方便超时计数，如果修改波特率，也必须更改这里相关的时间参数 */
            DelayUs(20);
            g_tcnt++;
            if (uart_rec_sign)
            {
                if (g_tcnt >= 43)
                {
                    /* 超过10个字节空闲时间没有新字节到来，且没有一个完整的数据包，就超时报错，根据波特率修改 */
                    /* 目前波特率为115200,一个字节时间为1s/11520 = 87us, 87us*10 / 20us = 43.5 */
                    /* 主机可以选择重发数据包，无影响 */
                    iap_rec_data_state = IAP_DATA_REC_STATE_WAIT_SOP1;
                    uart_rec_sign = 0;
                    iap_rsp_data[2] = 0xfe;
                    iap_rsp_data[3] = IAP_ERR_OVERTIME;
                    UART1_SendString(iap_rsp_data, sizeof(iap_rsp_data));
                }
            }
            else
            {
                if (g_tcnt > 6000000)
                {
                    /* 120秒没有数据，认为超时，跳到app，根据情况自行修改 */
                    jumpApp();
                }
            }
        }
    }
}


/*********************************************************************
 * @fn      my_memcpy
 *
 * @brief   数据拷贝函数,程序放ram中运行，提升速度
 *
 * @param   None.
 *
 * @return  None.
 */
__attribute__((section(".highcode")))
void my_memcpy(void *dst, const void *src, uint32_t l)
{
    uint32_t len = l;
    PUINT8 pdst = (PUINT8) dst;
    PUINT8 psrc = (PUINT8) src;
    while (len)
    {
        *pdst++ = *psrc++;
        len--;
    }
}
