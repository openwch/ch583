/********************************** (C) COPYRIGHT *******************************
 * File Name          : EXAM1.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/11
 * Description        : C语言的U盘创建长文件名文件例程
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

typedef struct __attribute__((packed)) _LONG_NAME
{                               //字节对齐
    uint8_t  LDIR_Ord;          /*长文件名的组号，如果为0X40则表示最后一个组*/
    uint16_t LDIR_Name1[5];     /*长文件名的前5个字节*/
    uint8_t  LDIR_Attr;         /*属性必须为ATTR_LONG_NAME*/
    uint8_t  LDIR_Type;         /* 为0表示长文件名的子项*/
    uint8_t  LDIR_Chksum;       /*短文件名的校验和*/
    uint16_t LDIR_Name2[6];     /*长文件名的6-11个字符*/
    uint8_t  LDIR_FstClusLO[2]; /*为0*/
    uint16_t LDIR_Name3[2];     /*长文件名的12-13各自。字符*/

} F_LONG_NAME; /*定义长文件名*/

typedef F_LONG_NAME *P_LONG_NAME;

#define MAX_LONG_NAME        4
#define FILE_LONG_NAME       MAX_LONG_NAME * 13 + 1
#define DATA_BASE_BUF_LEN    512

uint8_t DATA_BASE_BUF0[DATA_BASE_BUF_LEN];
uint8_t DATA_BASE_BUF1[DATA_BASE_BUF_LEN];

uint16_t LongFileName[FILE_LONG_NAME]; /*长文件名空间只存储文件名不存路径*/

/*********************************************************************
 * @fn      ChkSum
 *
 * @brief   计算短文件名的校验和
 *
 * @param   pDir1   - FAT数据区中文件目录信息
 *
 * @return  校验和
 */
unsigned char ChkSum(PX_FAT_DIR_INFO pDir1)
{
    unsigned char FcbNameLen;
    unsigned char Sum;
    Sum = 0;
    for(FcbNameLen = 0; FcbNameLen != 11; FcbNameLen++)
    {
        //if(pDir1->DIR_Name[FcbNameLen]==0x20)continue;
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + pDir1->DIR_Name[FcbNameLen];
    }
    return (Sum);
}

/*********************************************************************
 * @fn      mLDirCheck
 *
 * @brief   分析缓冲区的目录项和长文件名是否相同
 *
 * @param   pDir2   - FAT数据区中文件目录信息
 * @param   pLdir1  - 长文件名
 *
 * @return  返回00-15为找到长文件名相同的文件00-15表示对应短文件名在目录项的位置,
 *          返回0X80-8F表示分析到目录项的结尾,以后都是未用的目录项,返回0FF表示此扇区无匹配的目录项
 */
uint8_t mLDirCheck(PX_FAT_DIR_INFO pDir2, F_LONG_NAME *pLdir1)
{
    uint8_t      i, j, k, sum, nodir, nodir1;
    F_LONG_NAME *pLdir2;
    uint16_t    *pLName;
    for(i = 0; i != 16; i++)
    {
        if(pDir2->DIR_Name[0] == 0xe5)
        {
            pDir2 += 1;
            continue;
        } /*如果此项被删除继续分析下一目录*/ /*是被删除的文件名则跳过*/
        if(pDir2->DIR_Name[0] == 0x00)
        {
            return i | 0x80;
        } /*分析到以下空间没有文件名存在退出*/
        if((pDir2->DIR_Attr == 0x0f) | (pDir2->DIR_Attr == 0x8))
        {
            pDir2 += 1;
            continue;
        } /*如果找到的是卷标或者长文件名继续*/
        /*找到一个短文件名*/
        k = i - 1; /*长文件名项应在短文件名上面*/
        if(i == 0)
        {                    /*如果此短文件名在本扇区第一项*/
            pLdir2 = pLdir1; /*长文件名应在上一扇区的最后一项*/
            k = 15;          /*记录长文件名位置*/
            pLdir2 += 15;    /*偏移到结尾*/
        }
        else
            pLdir2 = (F_LONG_NAME *)(pDir2 - 1); /*取长文件名目录项*/
        sum = ChkSum(pDir2);                     /*计算累加和*/
        pLName = LongFileName;                   /*指项指定的长文件名*/
        nodir = 0;                               /*初始化标志*/
        nodir1 = 1;
        while(1)
        {
            if((pLdir2->LDIR_Ord != 0xe5) & (pLdir2->LDIR_Attr == ATTR_LONG_NAME) & (pLdir2->LDIR_Chksum == sum))
            { /*找到一个长文件名*/
                for(j = 0; j != 5; j++)
                {
                    if((pLdir2->LDIR_Name1[j] == 0x00) | (*pLName == 0))
                        continue; /*分析到长文件名结尾*/
                    if((pLdir2->LDIR_Name1[j] == 0xff) | (*pLName == 0))
                        continue; /*分析到长文件名结尾*/
                    if(pLdir2->LDIR_Name1[j] != *pLName)
                    { /*不等则设置标志*/
                        nodir = 1;
                        break;
                    }
                    pLName++;
                }
                if(nodir == 1)
                    break; /*文件名不同退出*/
                for(j = 0; j != 6; j++)
                {
                    if((pLdir2->LDIR_Name2[j] == 0x00) | (*pLName == 0))
                        continue;
                    if((pLdir2->LDIR_Name2[j] == 0xff) | (*pLName == 0))
                        continue;
                    if(*pLName != pLdir2->LDIR_Name2[j])
                    {
                        nodir = 1;
                        break;
                    }
                    pLName++;
                }
                if(nodir == 1)
                    break; /*文件名不同退出*/
                for(j = 0; j != 2; j++)
                {
                    if((pLdir2->LDIR_Name3[j] == 0x00) | (*pLName == 0))
                        continue;
                    if((pLdir2->LDIR_Name3[j] == 0xff) | (*pLName == 0))
                        continue;
                    if(*pLName != pLdir2->LDIR_Name3[j])
                    {
                        nodir = 1;
                        break;
                    }
                    pLName++;
                }
                if(nodir == 1)
                    break; /*文件名不同退出*/
                if((pLdir2->LDIR_Ord & 0x40) == 0x40)
                {
                    nodir1 = 0;
                    break;
                } /*找到长文件名，并且比较结束*/
            }
            else
                break; /*不是连续对应的长文件名退出*/
            if(k == 0)
            {
                pLdir2 = pLdir1;
                pLdir2 += 15;
                k = 15;
            }
            else
            {
                k = k - 1;
                pLdir2 -= 1;
            }
        }
        if(nodir1 == 0)
            return i; /*表示找到长文件名，返回短文件名在的目录项*/
        pDir2 += 1;
    }
    return 0xff; /*指搜索完这一个扇区没找到响应的长文件名*/
}

/*********************************************************************
 * @fn      mChkName
 *
 * @brief   检查上级子目录并打开
 *
 * @param   pJ      - 返回一组数据
 *
 * @return  状态
 */
uint8_t mChkName(unsigned char *pJ)
{
    uint8_t i, j;
    j = 0xFF;
    for(i = 0; i != sizeof(mCmdParam.Create.mPathName); i++)
    { /* 检查目录路径 */
        if(mCmdParam.Create.mPathName[i] == 0)
            break;
        if(mCmdParam.Create.mPathName[i] == PATH_SEPAR_CHAR1 || mCmdParam.Create.mPathName[i] == PATH_SEPAR_CHAR2)
            j = i; /* 记录上级目录 */
    }
    i = ERR_SUCCESS;
    if((j == 0) || ((j == 2) && (mCmdParam.Create.mPathName[1] == ':')))
    { /* 在根目录下创建 */
        mCmdParam.Open.mPathName[0] = '/';
        mCmdParam.Open.mPathName[1] = 0;
        i = CHRV3FileOpen(); /*打开根目录*/
        if(i == ERR_OPEN_DIR)
            i = ERR_SUCCESS; /* 成功打开上级目录 */
    }
    else
    {
        if(j != 0xFF)
        { /* 对于绝对路径应该获取上级目录的起始簇号 */
            mCmdParam.Create.mPathName[j] = 0;
            i = CHRV3FileOpen(); /* 打开上级目录 */
            if(i == ERR_SUCCESS)
                i = ERR_MISS_DIR; /* 是文件而非目录 */
            else if(i == ERR_OPEN_DIR)
                i = ERR_SUCCESS;                              /* 成功打开上级目录 */
            mCmdParam.Create.mPathName[j] = PATH_SEPAR_CHAR1; /* 恢复目录分隔符 */
        }
    }
    *pJ = j; /*指针中返回一组数据*/
    return i;
}

/*********************************************************************
 * @fn      CreatLongName
 *
 * @brief   创建并打开文件的长文件名，在短文件名空间输入路径以及参考短文件名，在长文件名空间输入该文件长文件名的UNICODE代码
 *
 * @return  返回00表示成功，并且在短文件名空间返回真实的短文件名，其他为不成功状态
 */
uint8_t CreatLongName()
{
    uint8_t         ParData[MAX_PATH_LEN]; /**/
    uint16_t        tempSec;               /*扇区偏移*/
    uint8_t         i, j, k, x, sum, y, z;
    P_LONG_NAME     pLDirName;
    PX_FAT_DIR_INFO pDirName, pDirName1;
    BOOL            FBuf;
    uint8_t        *pBuf1;
    uint16_t       *pBuf;
    CHRV3DirtyBuffer();
    for(k = 0; k != MAX_PATH_LEN; k++)
        ParData[k] = mCmdParam.Other.mBuffer[k];
    i = mChkName(&j);
    if(i == ERR_SUCCESS)
    {             /* 成功获取上级目录的起始簇号 */
        FBuf = 0; /*初始化*/
        tempSec = 0;
        DATA_BASE_BUF1[0] = 0xe5; /*无效上次缓冲区*/
        k = 0xff;
        while(1)
        {                                                                                        /*下面是读取并分析目录项*/
            pDirName = FBuf ? (PX_FAT_DIR_INFO)DATA_BASE_BUF1 : (PX_FAT_DIR_INFO)DATA_BASE_BUF0; /*短文件名指针指向缓冲区*/
            pLDirName = FBuf ? (P_LONG_NAME)DATA_BASE_BUF0 : (P_LONG_NAME)DATA_BASE_BUF1;
            mCmdParam.Read.mSectorCount = 1;                                     /*读取一扇区数据*/
            mCmdParam.Read.mDataBuffer = FBuf ? DATA_BASE_BUF1 : DATA_BASE_BUF0; /*当前处理的文件缓冲区,这里使用双向缓冲区，去处理文件名*/
            FBuf = !FBuf;                                                        /*缓冲区标志翻转*/
            i = CHRV3FileRead();
            if(mCmdParam.Read.mSectorCount == 0)
            {
                k = 0xff;
                break;
            }
            tempSec += 1;
            k = mLDirCheck(pDirName, pLDirName);
            z = k;
            z &= 0x0f;
            if(k != 0x0ff)
            {
                break;
            } /*找到文件或者找到文件结尾退出*/
        }
        if(k < 16)
        {
            pDirName += k; /*所找的文件短文件名在此目录项*/
            if(j != 0xff)
            {
                for(k = 0; k != j + 1; k++)
                    mCmdParam.Other.mBuffer[k] = ParData[k];
            }
            pBuf1 = &mCmdParam.Other.mBuffer[j + 1]; /*取文件名的地址*/
            //else pBuf1=&mCmdParam.Other.mBuffer;
            for(i = 0; i != 8; i++)
            {
                if(pDirName->DIR_Name[i] == 0x20)
                    continue;
                else
                {
                    *pBuf1 = pDirName->DIR_Name[i];
                    pBuf1++;
                }
            }
            if(pDirName->DIR_Name[i] != 0x20)
            {
                *pBuf1 = '.';
                pBuf1++;
            }
            for(; i != 11; i++)
            {
                if(pDirName->DIR_Name[i] == 0x20)
                    continue;
                else
                {
                    *pBuf1 = pDirName->DIR_Name[i];
                    pBuf1++;
                }

            } /*复制短文件名*/
            i = CHRV3FileClose();
            i = CHRV3FileCreate(); /*疑惑这里要不要恢复到刚进入此函数时的簇号*/
            printf("k<16\r\n");
            return i; /*创建文件,返回状态*/
        }
        else
        { /*表示目录项枚举到结束位置，要创建文件*/
            if(k == 0xff)
            {
                z = 00;
                tempSec += 1;
            }
            i = CHRV3FileClose();
            for(k = 0; k != MAX_PATH_LEN; k++)
                mCmdParam.Other.mBuffer[k] = ParData[k]; /*试创建文件短文件名*/
            for(x = 0x31; x != 0x3a; x++)
            { /*生成短文件名*/
                for(y = 0x31; y != 0x3a; y++)
                {
                    for(i = 0x31; i != 0x3a; i++)
                    {
                        mCmdParam.Other.mBuffer[j + 7] = i;
                        mCmdParam.Other.mBuffer[j + 6] = '~';
                        mCmdParam.Other.mBuffer[j + 5] = y;
                        mCmdParam.Other.mBuffer[j + 4] = x;
                        if(CHRV3FileOpen() != ERR_SUCCESS)
                            goto XAA1;
                        /**/
                    }
                }
            }
            i = 0xff;
            goto XBB;
        /*命名无法正确进行*/
        XAA1:

            i = CHRV3FileCreate();
            if(i != ERR_SUCCESS)
                return i; //{goto XCC;}			/*出错则不能继续进行*/
            for(k = 0; k != MAX_PATH_LEN; k++)
                ParData[k] = mCmdParam.Other.mBuffer[k]; /*试创建文件短文件名*/
            i = mChkName(&j);
            mCmdParam.Locate.mSectorOffset = tempSec - 1;
            i = CHRV3FileLocate();
            if(i != ERR_SUCCESS)
                return i; //{goto XCC;}			/*出错则不能继续进行*/
            mCmdParam.Read.mSectorCount = 1;
            mCmdParam.Read.mDataBuffer = DATA_BASE_BUF0;
            pDirName = (PX_FAT_DIR_INFO)DATA_BASE_BUF0;
            pDirName += z;       /*指向创建文件名的偏移*/
            i = CHRV3FileRead(); /*读取一个扇区的数据，取第一个目录项就是刚才创建的短文件名*/
            if(i != ERR_SUCCESS)
                return i; //{goto XCC;}				/*这里要做出错误处理*/

            for(i = 0; i != FILE_LONG_NAME; i++)
            {
                if(LongFileName[i] == 00)
                    break; /*计算长文件名的长度*/
            }
            for(k = i + 1; k != FILE_LONG_NAME; k++)
            { /*将无效长目录处填充*/
                LongFileName[k] = 0xffff;
            }
            k = i / 13; /*取长文件名组数*/
            i = i % 13;
            if(i != 0)
                k = k + 1; /*有余数则算一组*/
            i = k;
            k = i + z; /*z为短文件偏移,z-1为长文件偏移*/
            if(k < 16)
            {
                pDirName1 = (PX_FAT_DIR_INFO)DATA_BASE_BUF0;
                pDirName1 += k;
                sum = ChkSum(pDirName1); /*计算累加和*/
                pLDirName = (P_LONG_NAME)DATA_BASE_BUF0;
                pLDirName += (k - 1);
            }
            else if(k == 16)
            {
                pDirName1 = (PX_FAT_DIR_INFO)DATA_BASE_BUF1;
                pDirName1 += (k - 16);
                pLDirName = (F_LONG_NAME *)DATA_BASE_BUF0;
                pLDirName += (k - 1);
            }
            else if(k > 16)
            {
                pDirName1 = (PX_FAT_DIR_INFO)DATA_BASE_BUF1;
                pDirName1 += (k - 16);
                pLDirName = (F_LONG_NAME *)DATA_BASE_BUF1;
                pLDirName += (k - 1 - 16);
            }
            /*复制短文件名,将短文件名复制到指定区域*/
            pDirName1->DIR_NTRes = pDirName->DIR_NTRes;
            pDirName1->DIR_CrtTimeTenth = pDirName->DIR_CrtTimeTenth;
            pDirName1->DIR_CrtTime = pDirName->DIR_CrtTime;
            pDirName1->DIR_CrtDate = pDirName->DIR_CrtDate;
            pDirName1->DIR_LstAccDate = pDirName->DIR_LstAccDate;
            pDirName1->DIR_FstClusHI = pDirName->DIR_FstClusHI;
            pDirName1->DIR_WrtTime = pDirName->DIR_WrtTime;
            pDirName1->DIR_WrtDate = pDirName->DIR_WrtDate;
            pDirName1->DIR_FstClusLO = pDirName->DIR_FstClusLO;
            pDirName1->DIR_FileSize = pDirName->DIR_FileSize;
            pDirName1->DIR_Attr = pDirName->DIR_Attr;

            pDirName1->DIR_Name[0] = pDirName->DIR_Name[0];
            pDirName1->DIR_Name[1] = pDirName->DIR_Name[1];
            pDirName1->DIR_Name[2] = pDirName->DIR_Name[2];
            pDirName1->DIR_Name[3] = pDirName->DIR_Name[3];
            pDirName1->DIR_Name[4] = pDirName->DIR_Name[4];
            pDirName1->DIR_Name[5] = pDirName->DIR_Name[5];
            pDirName1->DIR_Name[6] = pDirName->DIR_Name[6];
            pDirName1->DIR_Name[7] = pDirName->DIR_Name[7];
            pDirName1->DIR_Name[8] = pDirName->DIR_Name[8];
            pDirName1->DIR_Name[9] = pDirName->DIR_Name[9];
            pDirName1->DIR_Name[10] = pDirName->DIR_Name[10];
            sum = ChkSum(pDirName1); /*计算累加和*/
            pBuf = LongFileName;     /*指向长文件名空间*/
            y = 1;
            if(k > 16)
            {
                for(i = 1; i != k - 16 + 1; i++)
                {
                    pLDirName->LDIR_Ord = y;
                    pLDirName->LDIR_Name1[0] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name1[1] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name1[2] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name1[3] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name1[4] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Attr = 0x0f;
                    pLDirName->LDIR_Type = 0;
                    pLDirName->LDIR_Chksum = sum;
                    pLDirName->LDIR_Name2[0] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name2[1] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name2[2] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name2[3] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name2[4] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name2[5] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_FstClusLO[0] = 0;
                    pLDirName->LDIR_FstClusLO[1] = 0;
                    pLDirName->LDIR_Name3[0] = *pBuf;
                    pBuf++;
                    pLDirName->LDIR_Name3[1] = *pBuf;
                    pBuf++;
                    pLDirName--;
                    y += 1;
                }
                k = 16;
                i = 0;
                pLDirName = (F_LONG_NAME *)DATA_BASE_BUF0;
                pLDirName += (k - 1);
            }
            if(k > 16)
                k = 16;
            for(i = 1; i != k - z; i++)
            {
                pLDirName->LDIR_Ord = y;
                pLDirName->LDIR_Name1[0] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name1[1] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name1[2] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name1[3] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name1[4] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Attr = 0x0f;
                pLDirName->LDIR_Type = 0;
                pLDirName->LDIR_Chksum = sum;
                pLDirName->LDIR_Name2[0] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name2[1] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name2[2] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name2[3] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name2[4] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name2[5] = *pBuf;
                pBuf++;
                pLDirName->LDIR_FstClusLO[0] = 0;
                pLDirName->LDIR_FstClusLO[1] = 0;
                pLDirName->LDIR_Name3[0] = *pBuf;
                pBuf++;
                pLDirName->LDIR_Name3[1] = *pBuf;
                pBuf++;
                pLDirName--;
                y += 1;
            }
            pLDirName->LDIR_Ord = y | 0x40;
            pLDirName->LDIR_Name1[0] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name1[1] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name1[2] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name1[3] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name1[4] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Attr = 0x0f;
            pLDirName->LDIR_Type = 0;
            pLDirName->LDIR_Chksum = sum;
            pLDirName->LDIR_Name2[0] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name2[1] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name2[2] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name2[3] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name2[4] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name2[5] = *pBuf;
            pBuf++;
            pLDirName->LDIR_FstClusLO[0] = 0;
            pLDirName->LDIR_FstClusLO[1] = 0;
            pLDirName->LDIR_Name3[0] = *pBuf;
            pBuf++;
            pLDirName->LDIR_Name3[1] = *pBuf;
            pBuf++;
            pBuf = (uint16_t *)pDirName1;
            pBuf += 16;

            if(pBuf < (uint16_t *)(DATA_BASE_BUF0 + 0x200))
            {
                i = 2;
                while(1)
                {
                    *pBuf = 0;
                    pBuf++;
                    if(pBuf == (uint16_t *)(DATA_BASE_BUF0 + 0x200))
                        break;
                }
                i++;
            }
            else if(pBuf < (uint16_t *)(DATA_BASE_BUF1 + 0x200))
            {
                i = 1;
                while(1)
                {
                    *pBuf = 0;
                    pBuf++;
                    if(pBuf == (uint16_t *)(DATA_BASE_BUF1 + 0x200))
                        break;
                }
                i++;
            }
            mCmdParam.Locate.mSectorOffset = tempSec - 1;
            CHRV3DirtyBuffer();
            i = CHRV3FileLocate();
            if(i != ERR_SUCCESS)
                return i;                    /*出错则不能继续进行*/
            mCmdParam.Read.mSectorCount = 1; /*下面重新*/
            mCmdParam.Read.mDataBuffer = DATA_BASE_BUF0;
            CHRV3DirtyBuffer();
            i = CHRV3FileWrite(); /*读取下一个扇区的数据，取第一个目录项就是刚才创建的短文件名*/
            CHRV3DirtyBuffer();
            if(i != ERR_SUCCESS)
                return i;                      /*这里要做出错误处理*/
            pBuf = (uint16_t *)DATA_BASE_BUF1; /**/
            if(*pBuf != 0)
            {
                mCmdParam.Read.mSectorCount = 1;
                mCmdParam.Read.mDataBuffer = DATA_BASE_BUF1;
                i = CHRV3FileWrite();
                CHRV3DirtyBuffer();
            }
            /*如果是在根目录下操作应关闭根目录*/
            /*下面还要打开文件*/
            i = CHRV3FileClose();
            for(k = 0; k != MAX_PATH_LEN; k++)
                mCmdParam.Other.mBuffer[k] = ParData[k]; /*试创建文件短文件名*/
            i = CHRV3FileOpen();                         /*打开创建的文件*/
            return i;
        }
    }
XBB:
{
    return i = 0xfe;
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
    uint8_t s, i;

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
                    //创建长文件名文件演示
                    printf("Create Long Name\n");
                    strcpy((uint8_t *)mCmdParam.Create.mPathName, "/TCBD~1.CSV"); /* 新文件名,在根目录下,中文文件名 */

                    LongFileName[0] = 0X0054; /*给出UNICODE的长文件名*/
                    LongFileName[1] = 0X0043; //TCBD_data_day.csv
                    LongFileName[2] = 0X0042;
                    LongFileName[3] = 0X0044;
                    LongFileName[4] = 0X005F;
                    LongFileName[5] = 0X0064;
                    LongFileName[6] = 0X0061;
                    LongFileName[7] = 0X0074;
                    LongFileName[8] = 0X0061;
                    LongFileName[9] = 0X005F;
                    LongFileName[10] = 0X0064;
                    LongFileName[11] = 0X0061;
                    LongFileName[12] = 0X0079;
                    LongFileName[13] = 0X002e;
                    LongFileName[14] = 0X0063;
                    LongFileName[15] = 0X0073;
                    LongFileName[16] = 0X0076;
                    LongFileName[17] = 0X0000;

                    s = CreatLongName(); /*创建长文件名*/
                    if(s != ERR_SUCCESS)
                        printf("Error: %02x\n", s);
                    else
                        printf("Creat end\n");
                }
            }
        }
        mDelaymS(100);  // 模拟单片机做其它事
        SetUsbSpeed(1); // 默认为全速
    }
}
