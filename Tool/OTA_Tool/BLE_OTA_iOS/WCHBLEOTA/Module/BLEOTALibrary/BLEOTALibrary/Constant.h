//
//  Constant.h
//  WCHBLELibrary
//
//  Created by 娟华 胡 on 2021/3/15.
//

#ifndef Constant_h
#define Constant_h

#define SERVICE_UUID @"FEE0"
#define READ_WRITE_CHARACTER_UUID @"FEE1"

#define SleepTimeGap 0.001

#define CONNECT_DOMAIN_INFO @"Info"
#define ERROR_INFO_CANCEL_SCAN  [NSError errorWithDomain:CONNECT_DOMAIN_INFO code:1 userInfo:@{NSUnderlyingErrorKey:@"已经在扫描请先取消"}]
#define Manager_State_Unsupported  [NSError errorWithDomain:CONNECT_DOMAIN_INFO code:1 userInfo:@{NSUnderlyingErrorKey:@"该设备不支持蓝牙"}]
#define Manager_State_PoweredOff  [NSError errorWithDomain:CONNECT_DOMAIN_INFO code:2 userInfo:@{NSUnderlyingErrorKey:@"蓝牙已关闭"}]
#define Manager_State_Resetting  [NSError errorWithDomain:CONNECT_DOMAIN_INFO code:2 userInfo:@{NSUnderlyingErrorKey:@"蓝牙正在重置"}]
#define Manager_State_Unauthorized  [NSError errorWithDomain:CONNECT_DOMAIN_INFO code:2 userInfo:@{NSUnderlyingErrorKey:@"蓝牙未授权"}]
#define Manager_State_Unknow  [NSError errorWithDomain:CONNECT_DOMAIN_INFO code:2 userInfo:@{NSUnderlyingErrorKey:@"蓝牙存在未知问题"}]

#define READ_MAX_REPEAT 20
#define READ_INTERVAL 0.1

#define TIMEOUT_REPEAT_NUM 10000
#define IAP_LEN 20

typedef NS_ENUM(NSUInteger, ChipType) {
    ChipType_UNKNOW = 0,
    ChipType_CH583 = 1,
    ChipType_CH573 = 2,
    ChipType_CH579 = 3,
};

typedef NS_ENUM(NSUInteger, ImageType) {
    ImageType_UNKNOW = 0,
    ImageType_A = 1,
    ImageType_B = 2,
};

//擦除命令
struct Erase {
    Byte cmd;             /*命令码0x81*/
    Byte len;             /*后续数据长度*/
    Byte addr[2];         /*擦除地址*/
    Byte block_num[2];    /*擦除块数*/
};

//结束命令
struct End {
    Byte cmd;             /*命令码0x83*/
    Byte len;             /*后续数据长度*/
    Byte status[2];       /*两字节状态，保留*/
};

//校验命令
struct Verify {
    Byte cmd;             /*命令码0x82*/
    Byte len;             /*后续数据长度*/
    Byte addr[2];         /*校验地址*/
    Byte buf[IAP_LEN - 4];/*校验数据*/
};

//编程命令
struct Program {
    Byte cmd;             /*命令码0x80*/
    Byte len;             /*后续数据长度*/
    Byte addr[2];         /*地址*/
    Byte buf[IAP_LEN - 4];/*后续数据*/
};

//编程命令
struct Info {
    Byte cmd;             /*命令码0x84*/
    Byte len;             /*后续数据长度*/
    Byte buf[IAP_LEN - 2];/*后续数据*/
};

struct Other {
    Byte buf[IAP_LEN];   /*接收数据包*/
};

#endif /* Constant_h */
