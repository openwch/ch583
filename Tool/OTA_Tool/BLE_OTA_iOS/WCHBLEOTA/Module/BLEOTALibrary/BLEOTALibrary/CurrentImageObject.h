//
//  CurrentImageObject.h
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/12.
//

#import <Foundation/Foundation.h>
#import "Constant.h"

@interface CurrentImageObject : NSObject

@property (nonatomic, assign) ImageType type;

@property (nonatomic, assign) ChipType chipType;

@property (nonatomic, assign) NSInteger offset;

@property (nonatomic, assign) NSInteger blockSize;

@property (nonatomic, strong) NSString *version;

+ (CurrentImageObject *)phraseImageFromResponse:(NSData *)data;

@end

@interface FileElement : NSObject

@end
