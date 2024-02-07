//
//  BLEOTAManager.h
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/7.
//

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import "CurrentImageObject.h"

//OTA升级进度类型
typedef NS_ENUM(NSUInteger, progressType) {
    progressType_Erase = 0,
    progressType_EraseSuccess = 1,
    progressType_EraseFailed = 2,
    progressType_Program = 3,
    progressType_ProgramWait = 4,
    progressType_ProgramSuccess = 5,
    progressType_ProgramFailed = 6,
    progressType_Verify = 7,
    progressType_VerifyWait = 8,
    progressType_VerifySuccess = 9,
    progressType_VerifyFailed = 10,
    progressType_End = 11,
};

typedef void(^writeDataCallBack)(NSInteger);
typedef void(^OTAProgressCallBack)(progressType);
typedef void(^rssiCallBack)(NSNumber *);

#pragma mark - 代理方法
@protocol BLEOTADelegate <NSObject>

//蓝牙状态回调
- (void)BLEManagerDidUpdateState:(NSError *)error;

//监听设备连接状态变更通知
- (void)BLEManagerDidPeripheralConnectUpateState:(CBPeripheral *)peripheral error:(NSError *)error;

//发现设备回调
- (void)BLEManagerDidDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary<NSString *, id> *)advertisementData RSSI:(NSNumber *)RSSI;

//发现服务回调
- (void)BLEManagerPeriphearl:(CBPeripheral *)peripheral services:(NSArray<CBService *> *)services error:(NSError *)error;

//发现服务特征回调
- (void)BLEManagerService:(CBService *)service characteristics:(NSArray<CBCharacteristic *> *)characteristics error:(NSError *)error;

//特征通道的值发生改变
- (void)BLEManagerUpdateValueForCharacteristic:(CBPeripheral *)peripheral Characteristic:(CBCharacteristic *)characteristic error:(NSError *)error;

@end

@interface BLEOTAManager : NSObject

//代理对象
@property (nonatomic, weak) id<BLEOTADelegate> delegate;

//开启调试信息
@property (nonatomic, assign)BOOL isDebug;

//可以根据传入的UUID扫描服务
@property (nonatomic, strong) NSArray<CBUUID *> * serviceUUIDS;

/*
 *@method 生成单列
 */
+ (instancetype) shareInstance;

/*
 *@method 开始扫描
 *@param serviceUUIDs 指定的UUID服务
 *@param scanRuler  扫描过滤规则
 */
- (void)startScan:(NSArray<CBUUID *> *)serviceUUIDs options:(NSDictionary *)scanRuler;

/*
 *@method 停止扫描
 */
- (void)stopScan;

/*
 *@method 连接设备
 */
- (void)connect:(CBPeripheral *)peripheral;

/*
 *@method 断开连接
 */
- (void)disconnect:(CBPeripheral *)peripheral;

/*
 *@method 获取当前连接的蓝牙设备硬件信息
 *@discussion 返回当前硬件设备的属性对象
 */
- (CurrentImageObject *)getCurrentImageInfo;

/*
 *@method 设置芯片类型
 *@discussion 对于芯片类型没有获取到的设备建议手动设置
 */
- (void)setChipType:(ChipType)type;

/*
 *@method hex文件数据转bin
 *@param  data hex文件的NSData
 *@discussion 返回bin格式的data
 */
- (NSData *)hex2BinData:(NSData *)data;

/*
 *@method 开始OTA升级
 *@param data             转换成bin后的NSData
 *@param eraseAddr        擦除地址
 *@param writeCallBack    升级步骤回调
 *@param progressCallBack 进度条回调
 */
- (void)startOTAUpdate:(NSData *)data eraseAddr:(NSInteger)eraseAddr writeCallBack:(writeDataCallBack)writeBlock progressCallBack:(OTAProgressCallBack)progressBlock;

/*
 *@method 取消OTA升级
 */
- (void)cancleOTAUpdate;

/*
 *@method 读取信号
 @param peripheral   外设对象
 @param rssiCallBack 信号强度回调
 */
- (void)readRSSI:(CBPeripheral *)peripheral rssiCallBack:(rssiCallBack)rssiBlock;

@end
