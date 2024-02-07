//
//  CurrentImageObject.m
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/12.
//

#import "CurrentImageObject.h"
#import "BLEOTACmd.h"

@implementation CurrentImageObject

+ (CurrentImageObject *)phraseImageFromResponse:(NSData *)data {
    if (data == nil || data.length != 20) {
        return nil;
    }
    
    Byte *response = (Byte *)[data bytes];
    CurrentImageObject *object = [[CurrentImageObject alloc] init];
    if (response[0] == 0x01) {
        object.type = ImageType_A;
    }else if (response[0] == 0x02) {
        object.type = ImageType_B;
    }else {
        object.type = ImageType_UNKNOW;
    }
    
    object.offset = [CurrentImageObject bytesToIntLittleEndian:response start:1];
    
    object.blockSize = (response[6] & 0xff) * 256 + (response[5] & 0xff);
    
    Byte b1 = response[7] & 0xff;
    Byte b2 = response[8] & 0xff;
    
    if (b1 == 0x83 & b2 == 0x00) {
        object.chipType = ChipType_CH583;
    }else if (b1 == 0x79 && b2 == 0x00) {
        object.chipType = ChipType_CH579;
    }else if (b1 == 0x73 && b2 == 0x00) {
        object.chipType = ChipType_CH573;
    }else {
        object.chipType = ChipType_UNKNOW;
    }
    
    [BLEOTACmd updateAddressBase:object.chipType];

    return object;
}

+ (int)bytesToIntLittleEndian:(Byte *)byte start:(UInt8)start {
    int s = 0;
    int s0 = byte[start + 3] & 0xff;
    int s1 = byte[start + 2] & 0xff;
    int s2 = byte[start + 1] & 0xff;
    int s3 = byte[start + 0] & 0xff;
    
    s0 <<= 24;
    s1 <<= 16;
    s2 <<= 8;
    
    s = s0 | s1 | s2 | s3;
    
    return s;
}

@end


@implementation FileElement

@end
