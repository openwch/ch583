//
//  CBAdvertisementData.h
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/7.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CBAdvertisementData : NSObject

+ (NSString *)getAdvertisementDataName:(NSString *)key;

+ (NSString *)getAdvertisementDataStringValue:(NSDictionary *)datas Key:(NSString *)key;

@end

NS_ASSUME_NONNULL_END
