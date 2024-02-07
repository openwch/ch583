//
//  BLEOTACmd.h
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/13.
//

#import <Foundation/Foundation.h>
#import "Constant.h"

NS_ASSUME_NONNULL_BEGIN

@interface BLEOTACmd : NSObject

+ (NSInteger)updateMTU:(NSInteger)mtu;

+ (void)updateAddressBase:(ChipType)type;

+ (NSData *)getImageInfoCommand;

+ (NSData *)getEraseCommand:(NSInteger)addr block:(NSInteger)block;

+ (NSData *)getProgrammeCommand:(NSInteger)addr data:(NSData *)bytes;

+ (NSData *)getVerifyCommand:(NSInteger)addr data:(NSData *)bytes;


+ (NSData *)getEndCommand;

@end

NS_ASSUME_NONNULL_END
