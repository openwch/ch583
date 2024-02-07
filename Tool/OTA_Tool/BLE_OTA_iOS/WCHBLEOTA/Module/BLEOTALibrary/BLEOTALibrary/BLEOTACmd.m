//
//  BLEOTACmd.m
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/13.
//

#import "BLEOTACmd.h"


#define CMD_ISP_PROGRAM 0x80
#define CMD_ISP_ERASE   0x81
#define CMD_ISP_VERIFY  0x82
#define CMD_ISP_END     0x83
#define CMD_ISP_INFO    0x84


NSInteger DATA_LEN = 20;
//地址需要除以它
//573，583的地址除以16，CH579除以4
NSInteger ADDRESS_BASE = 16;

@implementation BLEOTACmd

//为了提高发送速度编程前交换MTU
+ (NSInteger)updateMTU:(NSInteger)mtu {
    DATA_LEN = mtu;
    return DATA_LEN;
}

+ (void)updateAddressBase:(ChipType)type {
    if (type == ChipType_CH579 || type == ChipType_UNKNOW) {
        ADDRESS_BASE = 4;
    }else if (type == ChipType_CH573) {
        ADDRESS_BASE = 16;
    }else if (type == ChipType_CH583) {
        ADDRESS_BASE = 16;
    }
}

+ (NSData *)getImageInfoCommand {
    
//    struct Info info = {};
//    info.cmd = 0x84;
//    info.len = 0x12;
//
//    Byte buf[IAP_LEN - 2] = {};
//    memset(buf, 0x00, IAP_LEN - 2);
//    memmove(info.buf, buf, IAP_LEN - 2);
//
//    NSData *infoCmd = [[NSData alloc] initWithBytes:&info length:sizeof(info)];

    Byte byteBuffer[IAP_LEN] = {};
    memset(byteBuffer, 0, IAP_LEN);
    
    byteBuffer[0] = CMD_ISP_INFO;
    byteBuffer[1] = IAP_LEN - 2;
    
    NSData *data = [[NSData alloc] initWithBytes:byteBuffer length:IAP_LEN];
    return data;
}

+ (NSData *)getEraseCommand:(NSInteger)addr block:(NSInteger)block {

    Byte byteBuffer[IAP_LEN] = {};
    memset(byteBuffer, 0, IAP_LEN);
    
    byteBuffer[0] = CMD_ISP_ERASE;
    byteBuffer[1] = 0x00;
    byteBuffer[2] = addr / ADDRESS_BASE;
    byteBuffer[3] = ((addr / ADDRESS_BASE) >> 8);
    byteBuffer[4] = block;
    byteBuffer[5] = block >> 8;
    
    NSData *data = [[NSData alloc] initWithBytes:byteBuffer length:IAP_LEN];
    return data;
}

+ (NSData *)getProgrammeCommand:(NSInteger)addr data:(NSData *)bytes {
    Byte *byteBuffer = malloc(DATA_LEN);
    memset(byteBuffer, 0, DATA_LEN);
    
    byteBuffer[0] = CMD_ISP_PROGRAM;
    byteBuffer[1] = bytes.length;
    byteBuffer[2] = addr / ADDRESS_BASE;
    byteBuffer[3] = ((addr / ADDRESS_BASE) >> 8);
    
//    NSLog(@"编程地址:%ld offset:%ld", addr / ADDRESS_BASE, addr);
    
    Byte *dataByte = (Byte *)[bytes bytes];
    memmove(byteBuffer + 4, dataByte, bytes.length);
    
    NSData *data = [[NSData alloc] initWithBytes:byteBuffer length:DATA_LEN];
    free(byteBuffer);
    
    return data;
}


+ (NSData *)getVerifyCommand:(NSInteger)addr data:(NSData *)bytes  {
    Byte *byteBuffer = malloc(DATA_LEN);
    memset(byteBuffer, 0, DATA_LEN);
    
    byteBuffer[0] = CMD_ISP_VERIFY;
    byteBuffer[1] = bytes.length;
    byteBuffer[2] = addr / ADDRESS_BASE;
    byteBuffer[3] = ((addr / ADDRESS_BASE) >> 8);
    
//    NSLog(@"认证地址:%ld offset:%ld", addr / ADDRESS_BASE, addr);
    
    Byte *dataByte = (Byte *)[bytes bytes];
    memmove(byteBuffer + 4, dataByte, bytes.length);
    
    NSData *data = [[NSData alloc] initWithBytes:byteBuffer length:DATA_LEN];
    free(byteBuffer);
    
    return data;
}


+ (NSData *)getEndCommand {
    Byte byteBuffer[IAP_LEN] = {};
    memset(byteBuffer, 0, IAP_LEN);
    
    byteBuffer[0] = CMD_ISP_END;
    byteBuffer[1] = IAP_LEN - 2;
    
    NSData *data = [[NSData alloc] initWithBytes:byteBuffer length:IAP_LEN];
    return data;
}

@end
