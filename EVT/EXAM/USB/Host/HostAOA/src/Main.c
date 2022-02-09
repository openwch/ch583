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
// 各子程序返回状态码
#define ERR_SUCCESS          0x00  // 操作成功
#define ERR_USB_CONNECT      0x15  /* 检测到USB设备连接事件,已经连接 */
#define ERR_USB_DISCON       0x16  /* 检测到USB设备断开事件,已经断开 */
#define ERR_USB_BUF_OVER     0x17  /* USB传输的数据有误或者数据太多缓冲区溢出 */
#define ERR_USB_DISK_ERR     0x1F  /* USB存储器操作失败,在初始化时可能是USB存储器不支持,在读写操作中可能是磁盘损坏或者已经断开 */
#define ERR_USB_TRANSFER     0x20  /* NAK/STALL等更多错误码在0x20~0x2F */
#define ERR_USB_UNSUPPORT    0xFB  /*不支持的USB设备*/
#define ERR_USB_UNKNOWN      0xFE  /*设备操作出错*/
#define ERR_AOA_PROTOCOL     0x41  /*协议版本出错 */

__attribute__((aligned(4))) uint8_t RxBuffer[MAX_PACKET_SIZE]; // IN, must even address
__attribute__((aligned(4))) uint8_t TxBuffer[MAX_PACKET_SIZE]; // OUT, must even address
extern uint8_t                      Com_Buffer[];
//AOA获取协议版本
__attribute__((aligned(4))) const uint8_t GetProtocol[] = {0xc0, 0x33, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00};
//启动配件模式
__attribute__((aligned(4))) const uint8_t TouchAOAMode[] = {0x40, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
/* AOA相关数组定义 */
__attribute__((aligned(4))) const uint8_t Sendlen[] = {0, 4, 16, 35, 39, 53, 67};
//字符串ID,与手机APP相关的字符串信息
__attribute__((aligned(4))) uint8_t StringID[] = {
    'W',
    'C',
    'H',
    0x00,                                                                                                             //manufacturer name
    'W', 'C', 'H', 'U', 'A', 'R', 'T', 'D', 'e', 'm', 'o', 0x00,                                                      //model name
    0x57, 0x43, 0x48, 0x20, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x79, 0x20, 0x54, 0x65, 0x73, 0x74, 0x00, //description
    '1', '.', '0', 0x00,                                                                                              //version
    0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x63, 0x68, 0x2e, 0x63, 0x6e, 0,                                  //URI
    0x57, 0x43, 0x48, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x79, 0x31, 0x00                                //serial number
};
//应用索引字符串命令
__attribute__((aligned(4))) const uint8_t SetStringID[] = {0x40, 0x34, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x40, 0x34,
                                                           0x00, 0x00, 0x01, 0x00, 12, 0x00, 0x40, 0x34, 0x00, 0x00, 0x02,
                                                           0x00, 19, 0x00, 0x40, 0x34, 0x00, 0x00, 0x03, 0x00, 4, 0x00,
                                                           0x40, 0x34, 0x00, 0x00, 0x04, 0x00, 0x0E, 0x00, 0x40, 0x34,
                                                           0x00, 0x00, 0x05, 0x00, 0x0E, 0x00};

uint8_t TouchStartAOA(void); // 尝试启动AOA模式

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main()
{
    uint8_t s;
    uint8_t touchaoatm = 0;
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
            if((ThisUsbDev.DeviceVID == 0x18D1) && (ThisUsbDev.DevicePID & 0xff00) == 0x2D00)
            {
                PRINT("AOA Mode\n");
                ThisUsbDev.DeviceType = DEF_AOA_DEVICE;
            }
            else
            {                                        //如果不是AOA 配件模式，尝试启动配件模式.
                SetUsbSpeed(ThisUsbDev.DeviceSpeed); // 设置当前USB速度
                s = TouchStartAOA();
                if(s == ERR_SUCCESS)
                {
                    if(touchaoatm < 3) //尝试AOA启动次数限制
                    {
                        FoundNewDev = 1;
                        touchaoatm++;
                        mDelaymS(500); //部分安卓设备自动断开重连，所以此处最好有延时
                        continue;      //其实这里可以不用跳转，AOA协议规定，设备会自动重新接入总线。
                    }
                    //执行到这，说明可能不支持AOA，或是其他设备
                    PRINT("UNKOWN Device\n");
                    SetUsbSpeed(1);
                    while(1);
                }
            }
            //if ( s != ERR_SUCCESS ) 	return( s );
        }
    }
}

/*********************************************************************
 * @fn      TouchStartAOA
 *
 * @brief   尝试启动AOA模式
 *
 * @return  状态
 */
uint8_t TouchStartAOA(void)
{
    uint8_t len, s, i, Num;
    //获取协议版本号
    CopySetupReqPkg((uint8_t *)GetProtocol);
    s = HostCtrlTransfer(Com_Buffer, &len); // 执行控制传输
    if(s != ERR_SUCCESS)
    {
        return (s);
    }
    if(Com_Buffer[0] < 2)
        return ERR_AOA_PROTOCOL;

    //输出字符串
    for(i = 0; i < 6; i++)
    {
        Num = Sendlen[i];
        CopySetupReqPkg((uint8_t *)&SetStringID[8 * i]);
        s = HostCtrlTransfer(&StringID[Num], &len); // 执行控制传输
        if(s != ERR_SUCCESS)
        {
            return (s);
        }
    }

    CopySetupReqPkg((uint8_t *)TouchAOAMode);
    s = HostCtrlTransfer(Com_Buffer, &len); // 执行控制传输
    if(s != ERR_SUCCESS)
    {
        return (s);
    }
    return ERR_SUCCESS;
}
