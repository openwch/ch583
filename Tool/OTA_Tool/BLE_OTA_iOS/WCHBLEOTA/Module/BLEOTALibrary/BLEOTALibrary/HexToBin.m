//
//  HexToBin.m
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/22.
//

#import "HexToBin.h"

@implementation HexToBin

/*
 *把两个字符串转换成一个十六进制数值
 *pInChar 待转换的2个字符缓冲区
 *pOutChar 待换后是十六进制数值
 */
+ (BOOL)char2ToHex:(char *)pInChar outChar:(unsigned char *)pOutChar {
    unsigned char h, l;
    
    h = pInChar[0]; //高4位
    l = pInChar[1]; //低4位
    
    if (l >= '0' && l <= '9') {
        l = l - '0';
    }else if (l >= 'a' && l <= 'f') {
        l = l - 'a' + 0xa;
    }else if (l >= 'A' && l <= 'F') {
        l = l - 'A' + 0xa;
    }else {
        return NO;
    }
    
    if (h >= '0' && h <= '9') {
        h = h - '0';
    }else if (h >= 'a' && h <= 'f') {
        h = h - 'a' + 0xa;
    }else if (h >= 'A' && h <= 'F') {
        h = h - 'A' + 0xa;
    }else {
        return NO;
    }
    
    h <<= 4;
    h |= l;
    *pOutChar = h;
    return YES;
}


/*
 *HEX格式文件转换成BIN格式
 *Hexbuf
 */
+ (BOOL)HexToBin:(Byte *)Hexbuf hexLen:(u_long)iHexBufLen binBuf:(Byte *)Binbuf binLen:(u_long *)iBinLen {
    
    if (Hexbuf == NULL || Binbuf == NULL) {
        return NO;
    }
    
    unsigned char *hp;
    u_long StartPos;
    unsigned char DataBuf[256];
    ushort ExtAdr;
    ushort SegAdr;
    ushort OfsArd;
    u_long WriteAdr = 0, StartAddr = 0;
    BOOL bIsFrist = YES;
    u_long OffsetAdr = 0;
    
    memset(Binbuf, 0x00, iHexBufLen);
    
    StartPos = 0;
    ExtAdr = 0;
    SegAdr = 0;
    hp = (unsigned char *)Hexbuf;
    *iBinLen = 0;
    
    while (true) {
        if ((StartPos + 2) > iHexBufLen) {
            return NO;
        }
        
        if (hp[StartPos] == ':') {
            unsigned char i;
            u_long Len = 0;
            unsigned char checkSum;
            
            [HexToBin char2ToHex:(char *)(hp + StartPos + 1) outChar:(unsigned char *)&Len];
            
            for (checkSum = (unsigned char)Len, i = 0; i < (Len + 4); ++i) {
                [HexToBin char2ToHex:(char *)(hp + StartPos + 3 + i + i) outChar:(unsigned char *)(DataBuf + i)];
                checkSum += DataBuf[i];
            }
            
            if (checkSum != 0) {
                return NO;
            }
            
            switch (DataBuf[2]) {
                case 0:
                {
                    OfsArd = DataBuf[0] * 256 + DataBuf[1];
                    WriteAdr = ExtAdr * 65536 + SegAdr * 16 + OfsArd;
                    
                    if (bIsFrist) {
                        StartAddr = WriteAdr;
                        bIsFrist = NO;
                    }
                    
                    if (WriteAdr >= 0x08000000) {
                        
                        if (StartAddr != 0) {
                            WriteAdr -= StartAddr;
                        }else {
                            WriteAdr -= (ExtAdr * 65536);
                        }
                        
                    }else {
                        
                        if (OffsetAdr == 0) {
                            OffsetAdr = WriteAdr;
                        }else {
                            if (WriteAdr < OffsetAdr) {
                                OffsetAdr = WriteAdr;
                            }
                        }
                    }
                    
                    memcpy(&(((unsigned char *)Binbuf)[WriteAdr]), &DataBuf[3], Len);
                    StartPos += Len * 2 + 13;
                    
                    if ((WriteAdr + Len) > *iBinLen) {
                        *iBinLen = WriteAdr + Len;
                    }
                }
                    break;
                    
                case 2:
                {
                    SegAdr = DataBuf[3] * 256 + DataBuf[4];
                    StartPos += 17;
                }
                    break;
                    
                case 4:
                {
                    ExtAdr = DataBuf[3] * 256 + DataBuf[4];
                    StartPos += 17;
                }
                    break;
                    
                case 5:
                {
                    StartPos += 21;
                }
                    break;
                
                case 3:
                {
                    StartPos += 21;
                }
                    break;
                case 1:
                {
                    return YES;
                }
                    
                default:
                    return NO;
            }
        }
    }
    
    return NO;
}



@end
