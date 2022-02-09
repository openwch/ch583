/********************************** (C) COPYRIGHT *******************************
 * File Name          : EXAM1.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/03/11
 * Description        :
 C语言的U盘文件字节读写示例程序，文件指针偏移，修改文件属性，删除文件等操作，仅USB1支持
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

uint8_t buf[100]; //长度可以根据应用自己指定

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
    /* 遇到错误后,应该分析错误码以及CH554DiskStatus状态,例如调用CHRV3DiskReady查询当前U盘是否连接,如果U盘已断开那么就重新等待U盘插上再操作,
     建议出错后的处理步骤:
     1、调用一次CHRV3DiskReady,成功则继续操作,例如Open,Read/Write等
     2、如果CHRV3DiskReady不成功,那么强行将从头开始操作(等待U盘连接，CH554DiskReady等) */
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
    uint8_t  s, c, i;
    uint16_t TotalCount;

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
    while(1)
    {
        s = ERR_SUCCESS;
        if(R8_USB_INT_FG & RB_UIF_DETECT) // 如果有USB主机检测中断则处理
        {
            R8_USB_INT_FG = RB_UIF_DETECT; // 清连接中断标志
            s = AnalyzeRootHub();          // 分析ROOT-HUB状态
            if(s == ERR_USB_CONNECT)
                FoundNewDev = 1;
        }

        if(FoundNewDev || s == ERR_USB_CONNECT) // 有新的USB设备插入
        {
            FoundNewDev = 0;
            mDelaymS(200);        // 由于USB设备刚插入尚未稳定,故等待USB设备数百毫秒,消除插拔抖动
            s = InitRootDevice(); // 初始化USB设备
            if(s == ERR_SUCCESS)
            {
                // U盘操作流程：USB总线复位、U盘连接、获取设备描述符和设置USB地址、可选的获取配置描述符，之后到达此处，由CHRV3子程序库继续完成后续工作
                CHRV3DiskStatus = DISK_USB_ADDR;
                for(i = 0; i != 10; i++)
                {
                    printf("Wait DiskReady\n");
                    s = CHRV3DiskReady(); //等待U盘准备好
                    if(s == ERR_SUCCESS)
                    {
                        break;
                    }
                    else
                    {
                        printf("%02x\n", (uint16_t)s);
                    }
                    mDelaymS(50);
                }

                if(CHRV3DiskStatus >= DISK_MOUNTED)
                {
                    /* 读文件 */
//                    strcpy(mCmdParam.Open.mPathName, "/C51/CH573HFT.C"); //设置将要操作的文件路径和文件名/C51/CHRV3HFT.C
//                    s = CHRV3FileOpen();                                 //打开文件
//                    if(s == ERR_MISS_DIR || s == ERR_MISS_FILE)
//                    { //没有找到文件
//                        printf("没有找到文件\n");
//                    }
//                    else
//                    {                     //找到文件或者出错
//                        TotalCount = 100; //设置准备读取总长度100字节
//                        printf("读出的前%d个字符是:\n", TotalCount);
//                        while(TotalCount)
//                        { //如果文件比较大,一次读不完,可以再调用CHRV3ByteRead继续读取,文件指针自动向后移动
//                            if(TotalCount > (MAX_PATH_LEN - 1))
//                                c = MAX_PATH_LEN - 1; /* 剩余数据较多,限制单次读写的长度不能超过 sizeof( mCmdParam.Other.mBuffer ) */
//                            else
//                                c = TotalCount;                /* 最后剩余的字节数 */
//                            mCmdParam.ByteRead.mByteCount = c; /* 请求读出几十字节数据 */
//                            mCmdParam.ByteRead.mByteBuffer = &buf[0];
//                            s = CHRV3ByteRead();                         /* 以字节为单位读取数据块,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后读 */
//                            TotalCount -= mCmdParam.ByteRead.mByteCount; /* 计数,减去当前实际已经读出的字符数 */
//                            for(i = 0; i != mCmdParam.ByteRead.mByteCount; i++)
//                                printf("%c", mCmdParam.ByteRead.mByteBuffer[i]); /* 显示读出的字符 */
//                            if(mCmdParam.ByteRead.mByteCount < c)
//                            { /* 实际读出的字符数少于要求读出的字符数,说明已经到文件的结尾 */
//                                printf("\n");
//                                printf("文件已经结束\n");
//                                break;
//                            }
//                        }
//                        printf("Close\n");
//                        i = CHRV3FileClose(); /* 关闭文件 */
//                        mStopIfError(i);
//                    }
//                    /*如果希望从指定位置开始读写,可以移动文件指针 */
//                    mCmdParam.ByteLocate.mByteOffset = 608; //跳过文件的前608个字节开始读写
//                    CHRV3ByteLocate();
//                    mCmdParam.ByteRead.mByteCount = 5; //读取5个字节
//                    mCmdParam.ByteRead.mByteBuffer = &buf[0];
//                    CHRV3ByteRead(); //直接读取文件的第608个字节到612个字节数据,前608个字节被跳过
//                    //如果希望将新数据添加到原文件的尾部,可以移动文件指针
//                    CHRV3FileOpen();
//                    mCmdParam.ByteLocate.mByteOffset = 0xffffffff; //移到文件的尾部
//                    CHRV3ByteLocate();
//                    mCmdParam.ByteWrite.mByteCount = 13; //写入13个字节的数据
//                    CHRV3ByteWrite();                    //在原文件的后面添加数据,新加的13个字节接着原文件的尾部放置
//                    mCmdParam.ByteWrite.mByteCount = 2;  //写入2个字节的数据
//                    CHRV3ByteWrite();                    //继续在原文件的后面添加数据
//                    mCmdParam.ByteWrite.mByteCount = 0;  //写入0个字节的数据,实际上该操作用于通知程序库更新文件长度
//                    CHRV3ByteWrite();                    //写入0字节的数据,用于自动更新文件的长度,所以文件长度增加15,如果不这样做,那么执行CH554FileClose时也会自动更新文件长度

                    //创建文件演示
                    printf("Create\n");
                    strcpy((PCHAR)mCmdParam.Create.mPathName, "/NEWFILE.TXT"); /* 新文件名,在根目录下,中文文件名 */
                    s = CHRV3FileCreate();                                     /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
                    mStopIfError(s);
                    printf("ByteWrite\n");
                    //实际应该判断写数据长度和定义缓冲区长度是否相符，如果大于缓冲区长度则需要多次写入
                    i = sprintf((PCHAR)buf, "Note: \xd\xa这个程序是以字节为单位进行U盘文件读写,573简单演示功能。\xd\xa"); /*演示 */
                    for(c = 0; c < 10; c++)
                    {
                        mCmdParam.ByteWrite.mByteCount = i;    /* 指定本次写入的字节数 */
                        mCmdParam.ByteWrite.mByteBuffer = buf; /* 指向缓冲区 */
                        s = CHRV3ByteWrite();                  /* 以字节为单位向文件写入数据 */
                        mStopIfError(s);
                        printf("成功写入 %02X次\n", (uint16_t)c);
                    }

                    //演示修改文件属性
//                    printf("Modify\n");
//                    mCmdParam.Modify.mFileAttr = 0xff;                        //输入参数: 新的文件属性,为0FFH则不修改
//                    mCmdParam.Modify.mFileTime = 0xffff;                      //输入参数: 新的文件时间,为0FFFFH则不修改,使用新建文件产生的默认时间
//                    mCmdParam.Modify.mFileDate = MAKE_FILE_DATE(2015, 5, 18); //输入参数: 新的文件日期: 2015.05.18
//                    mCmdParam.Modify.mFileSize = 0xffffffff;                  // 输入参数: 新的文件长度,以字节为单位写文件应该由程序库关闭文件时自动更新长度,所以此处不修改
//                    i = CHRV3FileModify();                                    //修改当前文件的信息,修改日期
//                    mStopIfError(i);

                    printf("Close\n");
                    mCmdParam.Close.mUpdateLen = 1; /* 自动计算文件长度,以字节为单位写文件,建议让程序库关闭文件以便自动更新文件长度 */
                    i = CHRV3FileClose();
                    mStopIfError(i);

//                    strcpy((PCHAR)mCmdParam.Create.mPathName, "/NEWFILE.TXT"); /* 新文件名,在根目录下,中文文件名 */
//                    s = CHRV3FileOpen();                                       /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
//                    mStopIfError(s);

                    /* 删除某文件 */
//                    printf("Erase\n");
//                    strcpy(mCmdParam.Create.mPathName, "/OLD"); //将被删除的文件名,在根目录下
//                    i = CHRV3FileErase();                       //删除文件并关闭
//                    if(i != ERR_SUCCESS)
//                        printf("Error: %02X\n", (uint16_t)i); //显示错误
                }
            }
        }
        mDelaymS(100);  // 模拟单片机做其它事
        SetUsbSpeed(1); // 默认为全速
    }
}
