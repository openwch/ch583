/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/25
 * Description        : 模拟兼容HID设备
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"

#define DevEP0SIZE    0x40
// 设备描述符
const uint8_t MyDevDescr[] = {0x12,0x01,0x10,0x01,0x00,0x00,0x00,DevEP0SIZE,0x3d,0x41,0x07,0x21,0x00,0x00,0x00,0x00,0x00,0x01};
// 配置描述符
const uint8_t MyCfgDescr[] = {
    0x09,0x02,0x29,0x00,0x01,0x01,0x04,0xA0,0x23,               //配置描述符
    0x09,0x04,0x00,0x00,0x02,0x03,0x00,0x00,0x05,               //接口描述符
    0x09,0x21,0x00,0x01,0x00,0x01,0x22,0x22,0x00,               //HID类描述符
    0x07,0x05,0x81,0x03,0x40,0x00,0x01,              //端点描述符
    0x07,0x05,0x01,0x03,0x40,0x00,0x01               //端点描述符
};
/*字符串描述符略*/
/*HID类报表描述符*/
const uint8_t HIDDescr[] = {  0x06, 0x00,0xff,
                              0x09, 0x01,
                              0xa1, 0x01,                                                   //集合开始
                              0x09, 0x02,                                                   //Usage Page  用法
                              0x15, 0x00,                                                   //Logical  Minimun
                              0x26, 0x00,0xff,                                              //Logical  Maximun
                              0x75, 0x08,                                                   //Report Size
                              0x95, 0x40,                                                   //Report Counet
                              0x81, 0x06,                                                   //Input
                              0x09, 0x02,                                                   //Usage Page  用法
                              0x15, 0x00,                                                   //Logical  Minimun
                              0x26, 0x00,0xff,                                              //Logical  Maximun
                              0x75, 0x08,                                                   //Report Size
                              0x95, 0x40,                                                   //Report Counet
                              0x91, 0x06,                                                   //Output
                              0xC0};

/**********************************************************/
uint8_t        DevConfig, Ready = 0;
uint8_t        SetupReqCode;
uint16_t       SetupReqLen;
const uint8_t *pDescr;
uint8_t        Report_Value = 0x00;
uint8_t        Idle_Value = 0x00;
uint8_t        USB_SleepStatus = 0x00; /* USB睡眠状态 */

//HID设备中断传输中上传给主机4字节的数据
uint8_t HID_Buf[] = {0,0,0,0};

/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4))) uint8_t EP0_Databuf[64 + 64 + 64]; //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4))) uint8_t EP1_Databuf[64 + 64];      //ep1_out(64)+ep1_in(64)
__attribute__((aligned(4))) uint8_t EP2_Databuf[64 + 64];      //ep2_out(64)+ep2_in(64)
__attribute__((aligned(4))) uint8_t EP3_Databuf[64 + 64];      //ep3_out(64)+ep3_in(64)

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB 传输处理函数
 *
 * @return  none
 */
void USB_DevTransProcess(void)  //USB设备传输中断处理
{
    uint8_t len, chtype;        //len用于拷贝函数，chtype用于存放数据传输方向、命令的类型、接收的对象等信息
    uint8_t intflag, errflag = 0;   //intflag用于存放标志寄存器值，errflag用于标记是否支持主机的指令

    intflag = R8_USB_INT_FG;        //取得中断标识寄存器的值

    if(intflag & RB_UIF_TRANSFER)   //判断_INT_FG中的USB传输完成中断标志位。若有传输完成中断了，进if语句
    {
        if((R8_USB_INT_ST & MASK_UIS_TOKEN) != MASK_UIS_TOKEN) // 非空闲   //判断中断状态寄存器中的5:4位，查看令牌的PID标识。若这两位不是11（表示空闲），进if语句
        {
            switch(R8_USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))    //取得令牌的PID标识和设备模式下的3:0位的端点号。主机模式下3:0位是应答PID标识位
            // 分析操作令牌和端点号
            {                           //端点0用于控制传输。以下的端点0的IN和OUT令牌相应程序，对应控制传输的数据阶段和状态阶段。
                case UIS_TOKEN_IN:      //令牌包的PID为IN，5:4位为10。3:0位的端点号为0。IN令牌：设备给主机发数据。_UIS_：USB中断状态
                {                       //端点0为双向端点，用作控制传输。 “|0”运算省略了
                    switch(SetupReqCode)//这个值会在收到SETUP包时赋值。在后面会有SETUP包处理程序，对应控制传输的设置阶段。
                    {
                        case USB_GET_DESCRIPTOR:    //USB标准命令，主机从USB设备获取描述
                            len = SetupReqLen >= DevEP0SIZE ? DevEP0SIZE : SetupReqLen; // 本次包传输长度。最长为64字节，超过64字节的分多次处理，前几次要满包。
                            memcpy(pEP0_DataBuf, pDescr, len);//memcpy:内存拷贝函数，从(二号位)地址拷贝(三号位)字符串长度到(一号位)地址中
                            //DMA直接与内存相连，会检测到内存的改写，而后不用单片机控制就可以将内存中的数据发送出去。如果只是两个数组互相赋值，不涉及与DMA匹配的物理内存，就无法触发DMA。
                            SetupReqLen -= len;     //记录剩下的需要发送的数据长度
                            pDescr += len;          //更新接下来需要发送的数据的起始地址,拷贝函数用
                            R8_UEP0_T_LEN = len;    //端点0发送长度寄存器中写入本次包传输长度
                            R8_UEP0_CTRL ^= RB_UEP_T_TOG;   // 同步切换。IN方向（对于单片机就是T方向）的PID中的DATA0和DATA1切换
                            break;                  //赋值完端点控制寄存器的握手包响应（ACK、NAK、STALL），由硬件打包成符合规范的包，DMA自动发送
                        case USB_SET_ADDRESS:       //USB标准命令，主机为设备设置一个唯一地址，范围0～127，0为默认地址
                            R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | SetupReqLen;
                                    //7位地址+最高位的用户自定义地址（默认为1），或上“包传输长度”（这里的“包传输长度”在后面赋值成了地址位）
                            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                                    //R响应OUT事务ACK，T响应IN事务NAK。这个CASE分支里是IN方向，当DMA相应内存中，单片机没有数据更新时，回NAK握手包。
                            break;                                                  //一般程序里的OUT事务，设备会回包给主机，不响应NAK。

                        case USB_SET_FEATURE:       //USB标准命令，主机要求启动一个在设备、接口或端点上的特征
                            break;

                        default:
                            R8_UEP0_T_LEN = 0;      //状态阶段完成中断或者是强制上传0长度数据包结束控制传输（数据字段长度为0的数据包，包里SYNC、PID、EOP字段都有）
                            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                                    //R响应OUT事务ACK，T响应IN事务NAK。这个CASE分支里是OUT方向，当DMA相应内存中更新了数据且单片机验收正常时，回ACK握手包。
                            Ready = 1;
                            PRINT("Ready_STATUS = %d\n",Ready);
                            break;
                    }
                }
                break;

                case UIS_TOKEN_OUT:     //令牌包的PID为OUT，5:4位为00。3:0位的端点号为0。OUT令牌：主机给设备发数据。
                {                       //端点0为双向端点，用作控制传输。 “|0”运算省略了
                    len = R8_USB_RX_LEN;    //读取当前USB接收长度寄存器中存储的接收的数据字节数 //接收长度寄存器为各个端点共用，发送长度寄存器各有各的
                }
                break;

                case UIS_TOKEN_OUT | 1: //令牌包的PID为OUT，端点号为1
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)   //硬件会判断是否正确的同步切换数据包，同步切换正确，这一位自动置位
                    { // 不同步的数据包将丢弃
                        R8_UEP1_CTRL ^= RB_UEP_R_TOG;   //OUT事务的DATA同步切换。设定一个期望值。
                        len = R8_USB_RX_LEN;        //读取接收数据的字节数
                        DevEP1_OUT_Deal(len);       //发送长度为len的字节，自动回ACK握手包。自定义的程序。
                    }
                }
                break;

                case UIS_TOKEN_IN | 1: //令牌包的PID为IN，端点号为1
                    R8_UEP1_CTRL ^= RB_UEP_T_TOG;       //IN事务的DATA切换一下。设定将要发送的包的PID。
                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;    //当DMA中没有由单片机更新数据时，将T响应IN事务置为NAK。更新了就发出数据。
                    Ready = 1;
                    PRINT("Ready_IN_EP1 = %d\n",Ready);
                    break;
            }
            R8_USB_INT_FG = RB_UIF_TRANSFER;    //写1清零中断标志
        }

        if(R8_USB_INT_ST & RB_UIS_SETUP_ACT) // Setup包处理
        {
            R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
                        //R响应OUT事务期待为DATA1（DMA收到的数据包的PID要为DATA1，否则算数据错误要重传）和ACK（DMA相应内存中收到了数据，单片机验收正常）
                        //T响应IN事务设定为DATA1（单片机有数据送入DMA相应内存，以DATA1发送出去）和NAK（单片机没有准备好数据）。
            SetupReqLen = pSetupReqPak->wLength;    //数据阶段的字节数      //pSetupReqPak：将端点0的RAM地址强制转换成一个存放结构体的地址，结构体成员依次紧凑排列
            SetupReqCode = pSetupReqPak->bRequest;  //命令的序号
            chtype = pSetupReqPak->bRequestType;    //包含数据传输方向、命令的类型、接收的对象等信息

            len = 0;
            errflag = 0;
            if((pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) //判断命令的类型，如果不是标准请求，进if语句
            {
                /* 非标准请求 */
                /* 其它请求,如类请求，产商请求等 */
                if(pSetupReqPak->bRequestType & 0x40)   //取得命令中的某一位，判断是否为0，不为零进if语句
                {
                    /* 厂商请求 */
                }
                else if(pSetupReqPak->bRequestType & 0x20)  //取得命令中的某一位，判断是否为0，不为零进if语句
                {   //判断为HID类请求
                    switch(SetupReqCode)    //判断命令的序号
                    {
                        case DEF_USB_SET_IDLE: /* 0x0A: SET_IDLE */         //主机想设置HID设备特定输入报表的空闲时间间隔
                            Idle_Value = EP0_Databuf[3];
                            break; //这个一定要有

                        case DEF_USB_SET_REPORT: /* 0x09: SET_REPORT */     //主机想设置HID设备的报表描述符
                            break;

                        case DEF_USB_SET_PROTOCOL: /* 0x0B: SET_PROTOCOL */ //主机想设置HID设备当前所使用的协议
                            Report_Value = EP0_Databuf[2];
                            break;

                        case DEF_USB_GET_IDLE: /* 0x02: GET_IDLE */         //主机想读取HID设备特定输入报表的当前的空闲比率
                            EP0_Databuf[0] = Idle_Value;
                            len = 1;
                            break;

                        case DEF_USB_GET_PROTOCOL: /* 0x03: GET_PROTOCOL */     //主机想获得HID设备当前所使用的协议
                            EP0_Databuf[0] = Report_Value;
                            len = 1;
                            break;

                        default:
                            errflag = 0xFF;
                    }
                }
            }
            else    //判断为标准请求
            {
                switch(SetupReqCode)    //判断命令的序号
                {
                    case USB_GET_DESCRIPTOR:    //主机想获得标准描述符
                    {
                        switch(((pSetupReqPak->wValue) >> 8))   //右移8位，看原来的高8位是否为0，为1表示方向为IN方向，则进s-case语句
                        {
                            case USB_DESCR_TYP_DEVICE:  //不同的值代表不同的命令。主机想获得设备描述符
                            {
                                pDescr = MyDevDescr;    //将设备描述符字符串放在pDescr地址中，“获得标准描述符”这个case末尾会用拷贝函数发送
                                len = MyDevDescr[0];    //协议规定设备描述符的首字节存放字节数长度。拷贝函数会用到len参数
                            }
                            break;

                            case USB_DESCR_TYP_CONFIG:  //主机想获得配置描述符
                            {
                                pDescr = MyCfgDescr;    //将配置描述符字符串放在pDescr地址中，之后会发送
                                len = MyCfgDescr[2];    //协议规定配置描述符的第三个字节存放配置信息的总长
                            }
                            break;

                            case USB_DESCR_TYP_HID:     //主机想获得人机接口类描述符。此处结构体中的wIndex与配置描述符不同，意为接口号。
                                switch((pSetupReqPak->wIndex) & 0xff)       //取低八位，高八位抹去
                                {
                                    /* 选择接口 */
                                    case 0:
                                        pDescr = (uint8_t *)(&MyCfgDescr[18]);  //接口1的类描述符存放位置，待发送
                                        len = 9;
                                        break;

                                    default:
                                        /* 不支持的字符串描述符 */
                                        errflag = 0xff;
                                        break;
                                }
                                break;

                            case USB_DESCR_TYP_REPORT:  //主机想获得设备报表描述符
                            {
                                if(((pSetupReqPak->wIndex) & 0xff) == 0) //接口0报表描述符
                                {
                                    pDescr = HIDDescr; //数据准备上传
                                    len = sizeof(HIDDescr);
                                }
                                else
                                    len = 0xff; //本程序只有2个接口，这句话正常不可能执行
                            }
                            break;

                            case USB_DESCR_TYP_STRING:  //主机想获得设备字符串描述符
                            {
                                switch((pSetupReqPak->wValue) & 0xff)   //根据wValue的值传递字符串信息
                                {
                                    default:
                                        errflag = 0xFF; // 不支持的字符串描述符
                                        break;
                                }
                            }
                            break;

                            default:
                                errflag = 0xff;
                                break;
                        }
                        if(SetupReqLen > len)
                            SetupReqLen = len;      //实际需上传总长度
                        len = (SetupReqLen >= DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;   //最大长度为64字节
                        memcpy(pEP0_DataBuf, pDescr, len);  //拷贝函数
                        pDescr += len;
                    }
                    break;

                    case USB_SET_ADDRESS:       //主机想设置设备地址
                        SetupReqLen = (pSetupReqPak->wValue) & 0xff;    //将主机分发的位设备地址暂存在SetupReqLen中
                        break;                                          //控制阶段会赋值给设备地址参数

                    case USB_GET_CONFIGURATION: //主机想获得设备当前配置
                        pEP0_DataBuf[0] = DevConfig;    //将设备配置放进RAM
                        if(SetupReqLen > 1)
                            SetupReqLen = 1;    //将数据阶段的字节数置1。因为DevConfig只有一个字节
                        break;

                    case USB_SET_CONFIGURATION: //主机想设置设备当前配置
                        DevConfig = (pSetupReqPak->wValue) & 0xff;  //取低八位，高八位抹去
                        break;

                    case USB_CLEAR_FEATURE:     //关闭USB设备的特征/功能。可以是设备或是端点层面上的。
                    {
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) //判断是不是端点特征（清除端点停止工作的状态）
                        {
                            switch((pSetupReqPak->wIndex) & 0xff)   //取低八位，高八位抹去。判断索引
                            {       //16位的最高位判断数据传输方向，0为OUT，1为IN。低位为端点号。
                                case 0x81:      //清零_TOG和_T_RES这三位，并将后者写成_NAK，响应IN事务NAK表示无数据返回
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                    break;
                                case 0x01:      //清零_TOG和_R_RES这三位，并将后者写成_ACK，响应OUT事务ACK表示接收正常
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                                    break;
                                default:
                                    errflag = 0xFF; // 不支持的端点
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)  //判断是不是设备特征（用于设备唤醒）
                        {
                            if(pSetupReqPak->wValue == 1)   //唤醒标志位为1
                            {
                                USB_SleepStatus &= ~0x01;   //最低位清零
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                    }
                    break;

                    case USB_SET_FEATURE:       //开启USB设备的特征/功能。可以是设备或是端点层面上的。
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) //判断是不是端点特征（使端点停止工作）
                        {
                            /* 端点 */
                            switch(pSetupReqPak->wIndex)    //判断索引
                            {       //16位的最高位判断数据传输方向，0为OUT，1为IN。低位为端点号。
                                case 0x81:      //清零_TOG和_T_RES三位，并将后者写成_STALL，根据主机指令停止端点的工作
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                                    break;
                                case 0x01:      //清零_TOG和_R_RES三位，并将后者写成_STALL，根据主机指令停止端点的工作
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                                    break;
                                default:
                                    /* 不支持的端点 */
                                    errflag = 0xFF; // 不支持的端点
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)  //判断是不是设备特征（使设备休眠）
                        {
                            if(pSetupReqPak->wValue == 1)
                            {
                                USB_SleepStatus |= 0x01;    //设置睡眠
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                        break;

                    case USB_GET_INTERFACE:     //主机想获得接口当前工作的选择设置值
                        pEP0_DataBuf[0] = 0x00;
                        if(SetupReqLen > 1)
                            SetupReqLen = 1;    //将数据阶段的字节数置1。因为待传数据只有一个字节
                        break;

                    case USB_SET_INTERFACE:     //主机想激活设备的某个接口
                        break;

                    case USB_GET_STATUS:        //主机想获得设备、接口或是端点的状态
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) //判断是否为端点状态
                        {
                            /* 端点 */
                            pEP0_DataBuf[0] = 0x00;
                            switch(pSetupReqPak->wIndex)
                            {       //16位的最高位判断数据传输方向，0为OUT，1为IN。低位为端点号。
                                case 0x81:      //判断_TOG和_T_RES三位，若处于STALL状态，进if语句
                                    if((R8_UEP1_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01; //返回D0为1，表示端点被停止工作了。该位由SET_FEATURE和CLEAR_FEATURE命令配置。
                                    }
                                    break;

                                case 0x01:      //判断_TOG和_R_RES三位，若处于STALL状态，进if语句
                                    if((R8_UEP1_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)  //判断是否为设备状态
                        {
                            pEP0_DataBuf[0] = 0x00;
                            if(USB_SleepStatus)     //如果设备处于睡眠状态
                            {
                                pEP0_DataBuf[0] = 0x02;     //最低位D0为0，表示设备由总线供电，为1表示设备自供电。 D1位为1表示支持远程唤醒，为0表示不支持。
                            }
                            else
                            {
                                pEP0_DataBuf[0] = 0x00;
                            }
                        }
                        pEP0_DataBuf[1] = 0;    //返回状态信息的格式为16位数，高八位保留为0
                        if(SetupReqLen >= 2)
                        {
                            SetupReqLen = 2;    //将数据阶段的字节数置2。因为待传数据只有2个字节
                        }
                        break;

                    default:
                        errflag = 0xff;
                        break;
                }
            }
            if(errflag == 0xff) // 错误或不支持
            {
                //                  SetupReqCode = 0xFF;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
                Ready = 1;
                PRINT("Ready_Stall = %d\n",Ready);
            }
            else
            {
                if(chtype & 0x80)   // 上传。最高位为1，数据传输方向为设备向主机传输。
                {
                    len = (SetupReqLen > DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
                    SetupReqLen -= len;
                }
                else
                    len = 0;        // 下传。最高位为0，数据传输方向为主机向设备传输。
                R8_UEP0_T_LEN = len;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;     // 默认数据包是DATA1
            }

            R8_USB_INT_FG = RB_UIF_TRANSFER;    //写1清中断标识
        }
    }


    else if(intflag & RB_UIF_BUS_RST)   //判断_INT_FG中的总线复位标志位，为1触发
    {
        R8_USB_DEV_AD = 0;      //设备地址写成0，待主机重新分配给设备一个新地址
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;   //把端点0的控制寄存器，写成：接收响应响应ACK表示正常收到，发送响应NAK表示没有数据要返回
        R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_USB_INT_FG = RB_UIF_BUS_RST; //写1清中断标识
    }
    else if(intflag & RB_UIF_SUSPEND)   //判断_INT_FG中的总线挂起或唤醒事件中断标志位。挂起和唤醒都会触发此中断
    {
        if(R8_USB_MIS_ST & RB_UMS_SUSPEND)  //取得杂项状态寄存器中的挂起状态位，为1表示USB总线处于挂起态，为0表示总线处于非挂起态
        {
            Ready = 0;
            PRINT("Ready_Sleep = %d\n",Ready);
        } // 挂起     //当设备处于空闲状态超过3ms，主机会要求设备挂起（类似于电脑休眠）
        else    //挂起或唤醒中断被触发，又没有被判断为挂起
        {
            Ready = 1;
            PRINT("Ready_WeakUp = %d\n",Ready);
        } // 唤醒
        R8_USB_INT_FG = RB_UIF_SUSPEND; //写1清中断标志
    }
    else
    {
        R8_USB_INT_FG = intflag;    //_INT_FG中没有中断标识，再把原值写回原来的寄存器
    }
}

/*********************************************************************
 * @fn      DevHIDReport
 *
 * @brief   上报HID数据
 *
 * @return  0：成功
 *          1：出错
 */
void DevHIDReport(uint8_t data0,uint8_t data1,uint8_t data2,uint8_t data3)
{
    HID_Buf[0] = data0;
    HID_Buf[1] = data1;
    HID_Buf[2] = data2;
    HID_Buf[3] = data3;
    memcpy(pEP1_IN_DataBuf, HID_Buf, sizeof(HID_Buf));
    DevEP1_IN_Deal(sizeof(HID_Buf));
}

/*********************************************************************
 * @fn      DevWakeup
 *
 * @brief   设备模式唤醒主机
 *
 * @return  none
 */
void DevWakeup(void)
{
    R16_PIN_ANALOG_IE &= ~(RB_PIN_USB_DP_PU);
    R8_UDEV_CTRL |= RB_UD_LOW_SPEED;
    mDelaymS(2);
    R8_UDEV_CTRL &= ~RB_UD_LOW_SPEED;
    R16_PIN_ANALOG_IE |= RB_PIN_USB_DP_PU;
}

/*********************************************************************
 * @fn      DebugInit
 *
 * @brief   调试初始化
 *
 * @return  none
 */
void DebugInit(void)
{
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
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
    uint8_t s;
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    DebugInit();        //配置串口1用来prinft来debug
    printf("start\n");

    pEP0_RAM_Addr = EP0_Databuf;    //配置缓存区64字节。
    pEP1_RAM_Addr = EP1_Databuf;

    USB_DeviceInit();

    PFIC_EnableIRQ(USB_IRQn);       //启用中断向量
    mDelaymS(100);

    while(1)
    {//模拟传输4个字节的数据，实际传输根据用户需要自行修改
        if(Ready)
        {
            Ready = 0;
            DevHIDReport(0x05, 0x10, 0x20, 0x11);
        }
        mDelaymS(100);

        if(Ready)
        {
            Ready = 0;
            DevHIDReport(0x0A, 0x15, 0x25, 0x22);
        }
        mDelaymS(100);

        if(Ready)
        {
            Ready = 0;
            DevHIDReport(0x0E, 0x1A, 0x2A, 0x44);
        }
        mDelaymS(100);

        if(Ready)
        {
            Ready = 0;
            DevHIDReport(0x10, 0x1E, 0x2E, 0x88);
        }
        mDelaymS(100);
    }
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   端点1数据处理，收到数据后取反再发出去。用户自行更改。
 *
 * @return  none
 */
void DevEP1_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
    uint8_t i;

    for(i = 0; i < l; i++)
    {
        pEP1_IN_DataBuf[i] = ~pEP1_OUT_DataBuf[i];
    }
    DevEP1_IN_Deal(l);
}


/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB中断函数
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void USB_IRQHandler(void) /* USB中断服务程序,使用寄存器组1 */
{
    USB_DevTransProcess();
}
