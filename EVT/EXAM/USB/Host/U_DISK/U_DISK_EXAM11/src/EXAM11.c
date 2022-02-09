/********************************** (C) COPYRIGHT *******************************
 * File Name          : EXAM11.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/11
 * Description        : CH573 C语言的U盘目录文件枚举程序
 支持: FAT12/FAT16/FAT32
 注意包含 CHRV3UFI.LIB/USBHOST.C/DEBUG.C
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/** 不使用U盘文件系统库，需要在工程属性预编译中修改 DISK_LIB_ENABLE=0        */
/** U盘挂载USBhub下面，需要在工程属性预编译中修改 DISK_WITHOUT_USB_HUB=0  */

#include "CH58x_common.h"
#include "CHRV3UFI.H"

__attribute__((aligned(4))) uint8_t RxBuffer[MAX_PACKET_SIZE]; // IN, must even address
__attribute__((aligned(4))) uint8_t TxBuffer[MAX_PACKET_SIZE]; // OUT, must even address

/*********************************************************************
 * @fn      mStopIfError
 *
 * @brief   检查操作状态,如果错误则显示错误代码并停机
 *
 * @param   iError  - 错误码
 *
 * @return  none
 */
void mStopIfError(uint8_t iError)
{
    if(iError == ERR_SUCCESS)
    {
        return; /* 操作成功 */
    }
    printf("Error: %02X\n", (uint16_t)iError); /* 显示错误 */
    /* 遇到错误后,应该分析错误码以及CHRV3DiskStatus状态,例如调用CHRV3DiskReady查询当前U盘是否连接,如果U盘已断开那么就重新等待U盘插上再操作,
     建议出错后的处理步骤:
     1、调用一次CHRV3DiskReady,成功则继续操作,例如Open,Read/Write等
     2、如果CHRV3DiskReady不成功,那么强行将从头开始操作(等待U盘连接，CHRV3DiskReady等) */
    while(1)
    {
    }
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
    uint8_t  s, i;
    uint8_t *pCodeStr;
    uint16_t j;

    SetSysClock(CLK_SOURCE_PLL_60MHz);

    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
    PRINT("Start @ChipID=%02X \n", R8_CHIP_ID);

    pHOST_RX_RAM_Addr = RxBuffer;
    pHOST_TX_RAM_Addr = TxBuffer;
    USB_HostInit();
    CHRV3LibInit(); //初始化U盘程序库以支持U盘文件

    FoundNewDev = 0;
    printf("Wait Device In\n");
    while(1)
    {
        s = ERR_SUCCESS;
        if(R8_USB_INT_FG & RB_UIF_DETECT) // 如果有USB主机检测中断则处理
        {
            R8_USB_INT_FG = RB_UIF_DETECT; // 清连接中断标志
            s = AnalyzeRootHub();          // 分析ROOT-HUB状态
            if(s == ERR_USB_CONNECT)
            {
                FoundNewDev = 1;
            }
        }

        if(FoundNewDev || s == ERR_USB_CONNECT)
        {
            // 有新的USB设备插入
            FoundNewDev = 0;
            mDelaymS(200);        // 由于USB设备刚插入尚未稳定,故等待USB设备数百毫秒,消除插拔抖动
            s = InitRootDevice(); // 初始化USB设备
            if(s == ERR_SUCCESS)
            {
                printf("Start UDISK_demo @CHRV3UFI library\n");
                // U盘操作流程：USB总线复位、U盘连接、获取设备描述符和设置USB地址、可选的获取配置描述符，之后到达此处，由CHRV3子程序库继续完成后续工作
                CHRV3DiskStatus = DISK_USB_ADDR;
                for(i = 0; i != 10; i++)
                {
                    printf("Wait DiskReady\n");
                    s = CHRV3DiskReady();
                    if(s == ERR_SUCCESS)
                    {
                        break;
                    }
                    mDelaymS(50);
                }
                if(CHRV3DiskStatus >= DISK_MOUNTED) //U盘准备好
                {
                    /* 读文件 */
                    printf("Open\n");
                    strcpy((uint8_t *)mCmdParam.Open.mPathName, "/C51/CHRV3HFT.C"); //设置要操作的文件名和路径
                    s = CHRV3FileOpen();                                            //打开文件
                    if(s == ERR_MISS_DIR)
                    {
                        printf("不存在该文件夹则列出根目录所有文件\n");
                        pCodeStr = (uint8_t *)"/*";
                    }
                    else
                    {
                        pCodeStr = (uint8_t *)"/C51/*"; //列出\C51子目录下的的文件
                    }

                    printf("List file %s\n", pCodeStr);
                    for(j = 0; j < 10000; j++) //限定10000个文件,实际上没有限制
                    {
                        strcpy((uint8_t *)mCmdParam.Open.mPathName, (const uint8_t *)pCodeStr); //搜索文件名,*为通配符,适用于所有文件或者子目录
                        i = strlen((uint8_t *)mCmdParam.Open.mPathName);
                        mCmdParam.Open.mPathName[i] = 0xFF; //根据字符串长度将结束符替换为搜索的序号,从0到254,如果是0xFF即255则说明搜索序号在CHRV3vFileSize变量中
                        CHRV3vFileSize = j;                 //指定搜索/枚举的序号
                        i = CHRV3FileOpen();                //打开文件,如果文件名中含有通配符*,则为搜索文件而不打开
                        /* CHRV3FileEnum 与 CHRV3FileOpen 的唯一区别是当后者返回ERR_FOUND_NAME时那么对应于前者返回ERR_SUCCESS */
                        if(i == ERR_MISS_FILE)
                        {
                            break; //再也搜索不到匹配的文件,已经没有匹配的文件名
                        }
                        if(i == ERR_FOUND_NAME) //搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中
                        {
                            printf("  match file %04d#: %s\n", (unsigned int)j, mCmdParam.Open.mPathName); /* 显示序号和搜索到的匹配文件名或者子目录名 */
                            continue;                                                                      /* 继续搜索下一个匹配的文件名,下次搜索时序号会加1 */
                        }
                        else //出错
                        {
                            mStopIfError(i);
                            break;
                        }
                    }
                    i = CHRV3FileClose(); //关闭文件
                    printf("U盘演示完成\n");
                }
                else
                {
                    printf("U盘没有准备好 ERR =%02X\n", (uint16_t)s);
                }
            }
            else
            {
                printf("初始化U盘失败，请拔下U盘重试\n");
            }
        }
        mDelaymS(100);  // 模拟单片机做其它事
        SetUsbSpeed(1); // 默认为全速
    }
}
