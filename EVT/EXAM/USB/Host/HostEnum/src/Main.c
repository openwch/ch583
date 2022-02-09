/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/25
 * Description        : USB设备枚举
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"

__attribute__((aligned(4))) uint8_t RxBuffer[MAX_PACKET_SIZE]; // IN, must even address
__attribute__((aligned(4))) uint8_t TxBuffer[MAX_PACKET_SIZE]; // OUT, must even address

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main()
{
    uint8_t i, s, k, len, endp;
    uint16_t  loc;

    SetSysClock(CLK_SOURCE_PLL_60MHz);
    DelayMs(5);
    /* 开启电压监控 */
    PowerMonitor(ENABLE, HALevel_2V1);

    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
    PRINT("Start @ChipID=%02X\n", R8_CHIP_ID);

    pHOST_RX_RAM_Addr = RxBuffer;
    pHOST_TX_RAM_Addr = TxBuffer;
    USB_HostInit();
    PRINT("Wait Device In\n");
    while(1)
    {
        s = ERR_SUCCESS;
        if(R8_USB_INT_FG & RB_UIF_DETECT)
        { // 如果有USB主机检测中断则处理
            R8_USB_INT_FG = RB_UIF_DETECT;
            s = AnalyzeRootHub();
            if(s == ERR_USB_CONNECT)
                FoundNewDev = 1;
        }

        if(FoundNewDev || s == ERR_USB_CONNECT)
        { // 有新的USB设备插入
            FoundNewDev = 0;
            mDelaymS(200);        // 由于USB设备刚插入尚未稳定,故等待USB设备数百毫秒,消除插拔抖动
            s = InitRootDevice(); // 初始化USB设备
            if(s != ERR_SUCCESS)
            {
                PRINT("EnumAllRootDev err = %02X\n", (uint16_t)s);
            }
        }

        /* 如果下端连接的是HUB，则先枚举HUB */
        s = EnumAllHubPort(); // 枚举所有ROOT-HUB端口下外部HUB后的二级USB设备
        if(s != ERR_SUCCESS)
        { // 可能是HUB断开了
            PRINT("EnumAllHubPort err = %02X\n", (uint16_t)s);
        }

        /* 如果设备是鼠标 */
        loc = SearchTypeDevice(DEV_TYPE_MOUSE); // 在ROOT-HUB以及外部HUB各端口上搜索指定类型的设备所在的端口号
        if(loc != 0xFFFF)
        { // 找到了,如果有两个MOUSE如何处理?
            i = (uint8_t)(loc >> 8);
            len = (uint8_t)loc;
            SelectHubPort(len);                                                // 选择操作指定的ROOT-HUB端口,设置当前USB速度以及被操作设备的USB地址
            endp = len ? DevOnHubPort[len - 1].GpVar[0] : ThisUsbDev.GpVar[0]; // 中断端点的地址,位7用于同步标志位
            if(endp & USB_ENDP_ADDR_MASK)
            {                                                                                                       // 端点有效
                s = USBHostTransact(USB_PID_IN << 4 | endp & 0x7F, endp & 0x80 ? RB_UH_R_TOG | RB_UH_T_TOG : 0, 0); // 传输事务,获取数据,NAK不重试
                if(s == ERR_SUCCESS)
                {
                    endp ^= 0x80; // 同步标志翻转
                    if(len)
                        DevOnHubPort[len - 1].GpVar[0] = endp; // 保存同步标志位
                    else
                        ThisUsbDev.GpVar[0] = endp;
                    len = R8_USB_RX_LEN; // 接收到的数据长度
                    if(len)
                    {
                        PRINT("Mouse data: ");
                        for(i = 0; i < len; i++)
                        {
                            PRINT("x%02X ", (uint16_t)(RxBuffer[i]));
                        }
                        PRINT("\n");
                    }
                }
                else if(s != (USB_PID_NAK | ERR_USB_TRANSFER))
                {
                    PRINT("Mouse error %02x\n", (uint16_t)s); // 可能是断开了
                }
            }
            else
            {
                PRINT("Mouse no interrupt endpoint\n");
            }
            SetUsbSpeed(1); // 默认为全速
        }

        /* 如果设备是键盘 */
        loc = SearchTypeDevice(DEV_TYPE_KEYBOARD); // 在ROOT-HUB以及外部HUB各端口上搜索指定类型的设备所在的端口号
        if(loc != 0xFFFF)
        { // 找到了,如果有两个KeyBoard如何处理?
            i = (uint8_t)(loc >> 8);
            len = (uint8_t)loc;
            SelectHubPort(len);                                                // 选择操作指定的ROOT-HUB端口,设置当前USB速度以及被操作设备的USB地址
            endp = len ? DevOnHubPort[len - 1].GpVar[0] : ThisUsbDev.GpVar[0]; // 中断端点的地址,位7用于同步标志位
            if(endp & USB_ENDP_ADDR_MASK)
            {                                                                                                       // 端点有效
                s = USBHostTransact(USB_PID_IN << 4 | endp & 0x7F, endp & 0x80 ? RB_UH_R_TOG | RB_UH_T_TOG : 0, 0); // CH554传输事务,获取数据,NAK不重试
                if(s == ERR_SUCCESS)
                {
                    endp ^= 0x80; // 同步标志翻转
                    if(len)
                        DevOnHubPort[len - 1].GpVar[0] = endp; // 保存同步标志位
                    else
                        ThisUsbDev.GpVar[0] = endp;
                    len = R8_USB_RX_LEN; // 接收到的数据长度
                    if(len)
                    {
                        SETorOFFNumLock(RxBuffer);
                        PRINT("keyboard data: ");
                        for(i = 0; i < len; i++)
                        {
                            PRINT("x%02X ", (uint16_t)(RxBuffer[i]));
                        }
                        PRINT("\n");
                    }
                }
                else if(s != (USB_PID_NAK | ERR_USB_TRANSFER))
                {
                    PRINT("keyboard error %02x\n", (uint16_t)s); // 可能是断开了
                }
            }
            else
            {
                PRINT("keyboard no interrupt endpoint\n");
            }
            SetUsbSpeed(1); // 默认为全速
        }
    }
}
