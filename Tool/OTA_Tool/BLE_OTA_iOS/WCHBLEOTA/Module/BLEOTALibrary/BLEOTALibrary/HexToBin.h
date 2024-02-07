//
//  HexToBin.h
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/22.
//

#import <Foundation/Foundation.h>

//NSData *data = [[NSData alloc] initWithContentsOfURL:filePath];
//u_long size = data.length;
//Byte *biBuf = malloc(size * sizeof(Byte));
//[HexToBin HexToBin:(Byte *)[data bytes] hexLen:data.length binBuf:biBuf binLen:&size];
//NSData *binData = [[NSData alloc] initWithBytes:biBuf length:size];
//free(biBuf);

NS_ASSUME_NONNULL_BEGIN

@interface HexToBin : NSObject

+ (BOOL)HexToBin:(Byte *)Hexbuf hexLen:(u_long)iHexBufLen binBuf:(Byte *)Binbuf binLen:(u_long *)iBinLen;

@end

NS_ASSUME_NONNULL_END
