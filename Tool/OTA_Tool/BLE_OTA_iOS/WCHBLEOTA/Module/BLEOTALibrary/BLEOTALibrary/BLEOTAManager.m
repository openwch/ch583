//
//  BLEOTAManager.m
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/7.
//

#import "BLEOTAManager.h"
#import "Constant.h"
#import "BLEOTACmd.h"
#import "HexToBin.h"

@interface BLEOTAManager()<CBCentralManagerDelegate,CBPeripheralDelegate>

//蓝牙中心设备主机模式
@property(nonatomic, strong)CBCentralManager *centralManager;

//扫描选项
@property(nonatomic, strong)NSDictionary *scanOptions;

//============================================================
//已连接的外设
@property(nonatomic, strong)CBPeripheral *peripheral;

//连接中的外设
@property(nonatomic, strong)CBPeripheral *connectingPeripheral;

//============================================================
@property(nonatomic, strong)NSOperationQueue *writeQueue;
@property(nonatomic, strong)NSOperationQueue *cmdQueue;

//最大的写入数据
@property(nonatomic, assign)NSInteger writeWithoutMaxLen;
@property(nonatomic, assign)NSInteger writeWithMaxLen;

//无响应写入
@property(nonatomic, assign)BOOL canSendWrite;

//==================================================

@property (nonatomic, copy) rssiCallBack rssiBlock;

//OTA升级通道
@property (nonatomic, strong) CBCharacteristic *otaCharacter;
//是否正在读
@property (nonatomic, assign) BOOL readResponse;
//是否正在写
@property (nonatomic, assign) BOOL writeResponse;
//擦除地址是否成功
@property (nonatomic, assign) BOOL eraseResponse;
//编程是否成功
@property (nonatomic, assign) BOOL programRespone;
//认证是否成功
@property (nonatomic, assign) BOOL verifyRespone;

@property (nonatomic, assign) BOOL cancleUpdate;
//信号量
@property (nonatomic, strong) dispatch_semaphore_t semaphore;
//芯片信息
@property (nonatomic, strong) CurrentImageObject *currentImageObject;

@end

@implementation BLEOTAManager

//生成单列
+ (instancetype) shareInstance {
    static BLEOTAManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[super allocWithZone:NULL] init];
    });
    
    return instance;
}

// 如果别人init了这个类，就会创建一个新的对象，要保证永远都只为单例对象分配一次内存空间，写法如下：
+ (id)allocWithZone:(struct _NSZone *)zone {
    return [BLEOTAManager shareInstance];
}

- (id)copyWithZone:(struct _NSZone *)zone {
    return [BLEOTAManager shareInstance];
}

//初始化方法
- (instancetype)init {
    self = [super init];
    if (self) {
        [self setupBLE];
    }
    return self;
}

//设置蓝牙中心管理器
- (void)setupBLE {
    if (self.centralManager == nil) {
        NSDictionary *option=@{CBCentralManagerOptionShowPowerAlertKey:@NO};
        CBCentralManager *manager = [[CBCentralManager alloc] initWithDelegate:self queue:nil options:option];
        self.centralManager = manager;
    }
    self.isDebug = YES;
    self.readResponse = YES;
    self.writeResponse = YES;
    self.cancleUpdate = NO;
    
    if (nil == self.cmdQueue) {
        self.cmdQueue = [[NSOperationQueue alloc] init];
    }
    self.cmdQueue.maxConcurrentOperationCount = 1;
}

#pragma mark - 扫描外设

- (void)startScan:(NSArray<CBUUID *> *)serviceUUIDs options:(NSDictionary *)scanRuler; {
    [self scanBLE:serviceUUIDs options:scanRuler];
}

//扫描方法
- (void)scanBLE:(nullable NSArray<CBUUID *> *)serviceUUIDs options:(nullable NSDictionary<NSString *, id> *)options{
    
    if (nil == self.centralManager) {
        [self setupBLE];
    }
    
    if (self.centralManager.isScanning) {
        [self BLEManagerState:ERROR_INFO_CANCEL_SCAN];
        return;
    }
    //保存扫描规则
    self.scanOptions = options;
    //存在已连接的设备
    if (self.peripheral != nil && self.peripheral.state == CBPeripheralStateConnected) {
        //断开当前设备
        [self.centralManager cancelPeripheralConnection:self.peripheral];
    }
    [self.centralManager scanForPeripheralsWithServices:serviceUUIDs options:options];
}

//停止扫描
- (void)stopScan {
    if (self.centralManager.isScanning) {
        [self.centralManager stopScan];
    }
}

- (NSArray<CBPeripheral *> *)retrievePeripheralsWithIdentifiers:(NSArray<NSUUID *> *)identifiers {
    return [self.centralManager retrievePeripheralsWithIdentifiers:identifiers];
}

#pragma mark - 发现外设-连接-断开
- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary<NSString *, id> *)advertisementData RSSI:(NSNumber *)RSSI {

    NSString *log = [[NSString alloc] initWithFormat:@"发现设备：%@ 信号：%@ advertisementData:%@", peripheral, RSSI, advertisementData];
    
    if (_isDebug){
        NSLog(@"%@", log);
    }
    
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerDidDiscoverPeripheral:advertisementData:RSSI:)]) {
        [self.delegate BLEManagerDidDiscoverPeripheral:peripheral advertisementData:advertisementData RSSI:RSSI];
    }
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral {
 
    NSString *log = [[NSString alloc] initWithFormat:@"连接成功%@", peripheral];
    
    if (_isDebug) {
        NSLog(@"%@", log);
    }
    
    //取消延迟检测器
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(checkConnectinState) object:nil];
    [self.centralManager stopScan];
    self.peripheral = peripheral;
    self.connectingPeripheral = nil;
    //返回连接的外设回调
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerDidPeripheralConnectUpateState:error:)]){
        [self.delegate BLEManagerDidPeripheralConnectUpateState:peripheral error:nil];
    }
    //开始扫描服务
    [self startScanServiceForPeripheral:peripheral];
}

- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {

    NSString *log = [[NSString alloc] initWithFormat:@"连接失败%@, error:%@", peripheral, error];
    if (_isDebug) {
        NSLog(@"%@", log);
    }
    
    if (self.connectingPeripheral == peripheral) {
        error =  [[NSError alloc] initWithDomain:@"连接超时" code:1001 userInfo:@{NSUnderlyingErrorKey:@"连接超时"}];
        self.connectingPeripheral = nil;
    }
    
    if (self.peripheral == peripheral) {
        error =  [[NSError alloc] initWithDomain:@"已断开连接" code:1001 userInfo:@{NSUnderlyingErrorKey:@"已断开连接"}];
        self.peripheral = nil;
    }
    
    //返回连接的外设回调
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerDidPeripheralConnectUpateState:error:)]){
        [self.delegate BLEManagerDidPeripheralConnectUpateState:peripheral error:error];
    }
}

//连接设备
- (void)connect:(CBPeripheral *)peripheral {

    if (peripheral == nil) {
        NSString *log = [[NSString alloc] initWithFormat:@"没有连接的外设"];
        if (_isDebug) {
            NSLog(@"%@", log);
        }
        return;
    }
    //断开已经连接的设备
    [self disconnect:self.peripheral];
    
    self.connectingPeripheral = peripheral;
    //发起连接
    self.peripheral = nil;
    [self.centralManager connectPeripheral:peripheral options:nil];
    //延时执行，获取连接状态
//    __weak typeof(self) weakSelf = self;
    [self performSelector:@selector(checkConnectinState) withObject:nil afterDelay:2.0f];
}

//断开连接
- (void)disconnect:(CBPeripheral *)peripheral {
    
    if (peripheral == nil) {
        NSString *log = [[NSString alloc] initWithFormat:@"没有连接的外设"];
        if (_isDebug) {
            NSLog(@"%@", log);
        }
        return;
    }

    NSString *log = [[NSString alloc] initWithFormat:@"关闭外设连接"];
    if (_isDebug) {
        NSLog(@"%@", log);
    }
    [self.centralManager cancelPeripheralConnection:peripheral];
    self.currentImageObject = nil;
}

//检查连接状体
-(void)checkConnectinState {
    if (self.peripheral == nil) {
        //连接未成功
        NSString *log = [[NSString alloc] initWithFormat:@"外设连接超时、取消连接"];
        if (_isDebug){
            NSLog(@"%@", log);
        }
        if (self.connectingPeripheral != nil) {
            [self.centralManager cancelPeripheralConnection:self.connectingPeripheral];
        }
    }
}

- (CurrentImageObject *)getCurrentImageInfo {
    //连接成功
    return self.currentImageObject;
}


//获取芯片参数信息
- (void)sendImageCommand {
    
    if (!self.readResponse || !self.writeResponse) {
        return;
    }
    
    __weak typeof(self) weakSelf = self;
    NSBlockOperation *operation = [NSBlockOperation blockOperationWithBlock:^{
        NSData *data =  [BLEOTACmd getImageInfoCommand];
        [weakSelf writeData:data peripheral:weakSelf.peripheral writeForChar:weakSelf.otaCharacter type:CBCharacteristicWriteWithResponse];
        
        for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
            [NSThread sleepForTimeInterval:0.001];
            if (weakSelf.writeResponse) {
                break;
            }
        }
    }];
    
    operation.completionBlock = ^{
        
        if (weakSelf.writeResponse) {

            [weakSelf readData:weakSelf.peripheral readValueForChar:weakSelf.otaCharacter];
            for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
                [NSThread sleepForTimeInterval:0.001];
                if (weakSelf.readResponse) {
                    break;
                }
            }
            if (weakSelf.readResponse) {
                NSData *valueData = [weakSelf.otaCharacter value];
                weakSelf.currentImageObject = [CurrentImageObject phraseImageFromResponse:valueData];
            }
        }
        weakSelf.readResponse = YES;
        weakSelf.writeResponse = YES;
    };
    
    [self.cmdQueue addOperation:operation];
    
}

//发送擦除命令
- (void)sendEraseCmmand:(NSInteger)addr block:(NSInteger)block {
    
    if (!self.readResponse || !self.writeResponse) {
        return;
    }
    
    self.eraseResponse = NO;
    
    __weak typeof(self) weakSelf = self;
    NSBlockOperation *operation = [NSBlockOperation blockOperationWithBlock:^{
        if (weakSelf.cancleUpdate) {
            return;
        }
        
        NSData *data =  [BLEOTACmd getEraseCommand:addr block:block];
        [weakSelf writeData:data peripheral:weakSelf.peripheral writeForChar:weakSelf.otaCharacter type:CBCharacteristicWriteWithResponse];
        
        for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
            [NSThread sleepForTimeInterval:0.001];
            if (weakSelf.writeResponse) {
                break;
            }
        }
    }];
    
    operation.completionBlock = ^{
        
        if (weakSelf.writeResponse) {
            
            if (weakSelf.currentImageObject.chipType == ChipType_CH579 || weakSelf.currentImageObject.chipType == ChipType_UNKNOW) {
                [NSThread sleepForTimeInterval:2];
            }
      
            [weakSelf readData:weakSelf.peripheral readValueForChar:weakSelf.otaCharacter];
            for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
                [NSThread sleepForTimeInterval:0.001];
                if (weakSelf.readResponse) {
                    break;
                }
            }
            if (weakSelf.readResponse) {
                NSData *valueData = [weakSelf.otaCharacter value];
                Byte *response = (Byte *)[valueData bytes];
                if (response != nil && valueData.length > 0 && response[0] == 0x00) {
                    weakSelf.eraseResponse = YES;
                }
            }
        }
        weakSelf.readResponse = YES;
        weakSelf.writeResponse = YES;
    };
    
    [self.cmdQueue addOperation:operation];
    
}

//发送结束命令
- (void)sendEndCmmand:(OTAProgressCallBack)progressCallBack {
    
    if (!self.readResponse || !self.writeResponse) {
        return;
    }
    
    self.eraseResponse = NO;
    
    __weak typeof(self) weakSelf = self;
    NSBlockOperation *operation = [NSBlockOperation blockOperationWithBlock:^{
        if (weakSelf.cancleUpdate) {
            return;
        }
        
        NSData *data =  [BLEOTACmd getEndCommand];
        [weakSelf writeData:data peripheral:weakSelf.peripheral writeForChar:weakSelf.otaCharacter type:CBCharacteristicWriteWithResponse];
        
        for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
            [NSThread sleepForTimeInterval:0.001];
            if (weakSelf.writeResponse) {
                break;
            }
        }
    }];
    
    operation.completionBlock = ^{
        weakSelf.writeResponse = YES;
        if (nil != progressCallBack) {
            progressCallBack(progressType_End);
        }
    };
    
    [self.cmdQueue addOperation:operation];
}

- (void)udpataDataStream:(NSData *)data startAddr:(NSInteger)startAddr writeCallBack:(writeDataCallBack)writeBlock progressCallBack:(OTAProgressCallBack)progressBlock isProgram:(BOOL)isProgram {
    
    //升级数据为空，不存在
    if (data == nil) {
        return;
    }
    
    if (self.cancleUpdate) {
        return;
    }
    
    //数据下发队列
    self.writeQueue = [[NSOperationQueue alloc] init];
    
    self.writeQueue.maxConcurrentOperationCount = 1;
    [self.writeQueue cancelAllOperations];
    [self.writeQueue setSuspended:NO];

    //初始化信号量
    self.semaphore = dispatch_semaphore_create(0);
    
    //是否可编程
    if (isProgram) {
        self.programRespone = NO;
    }else {
        self.verifyRespone = NO;
    }
   

    NSInteger maxLen;
    //蓝牙一包数据的最大长度
    maxLen = [BLEOTACmd updateMTU:[self getSpecificMaxPacketForOTAAlign:self.writeWithoutMaxLen]] - 4;
    
    NSInteger dataLen = data.length;
    NSInteger writeNum = 0;
    __block NSInteger offset = 0;
    
    //分解数据包
    if (dataLen <= maxLen) {
        writeNum = 1;
    }else {
        writeNum = ceilf(dataLen / (float)maxLen);
    }
    
    if (isProgram) {
        //编程流程
        if (nil != progressBlock) {
            progressBlock(progressType_Program);
        }
    }else {
        //校验流程
        if (nil != progressBlock) {
            progressBlock(progressType_Verify);
        }
    }

    @autoreleasepool {
    
        //进行数据异步发包
        for (int i = 0; i < writeNum; i++) {
            
            NSData *writeData = nil;
            if (dataLen >= (i + 1) * maxLen) {
                writeData = [data subdataWithRange:NSMakeRange(i * maxLen, maxLen)];
            }else {
                NSInteger spaceSize = dataLen - i * maxLen;
                writeData = [data subdataWithRange:NSMakeRange(i * maxLen, spaceSize)];
            }
            
            __weak typeof(self) weakSelf = self;
            NSOperation *operation = [NSBlockOperation blockOperationWithBlock:^{
                
                if (weakSelf.cancleUpdate) {
                    [weakSelf.writeQueue cancelAllOperations];
                    return;
                }
                
                [weakSelf.writeQueue setSuspended:YES];
                weakSelf.canSendWrite = [weakSelf.peripheral canSendWriteWithoutResponse];
                
                if (!weakSelf.canSendWrite) {
                    //等待无响应写数据超时
                    for (int i = 1; i <= TIMEOUT_REPEAT_NUM; i++) {
                        [NSThread sleepForTimeInterval:SleepTimeGap];
                        weakSelf.canSendWrite = [weakSelf.peripheral canSendWriteWithoutResponse];
                        if (weakSelf.canSendWrite) {
                            //无响应写结果返回
                            break;
                        }
                        
                        //无响应写超时
                        if (i == TIMEOUT_REPEAT_NUM) {
                            [weakSelf.writeQueue cancelAllOperations];
                            if (isProgram) {
                                if (nil != progressBlock) {
                                    progressBlock(progressType_ProgramFailed);
                                }
                            }else {
                                if (nil != progressBlock) {
                                    progressBlock(progressType_VerifyFailed);
                                }
                            }
                            dispatch_semaphore_signal(weakSelf.semaphore);
                            return;
                        }
                    }
                }

                //开始组包
                NSData *sendData;
                
                if (isProgram) {
                    sendData = [BLEOTACmd getProgrammeCommand:offset + startAddr data:writeData];
                }else {
                    sendData = [BLEOTACmd getVerifyCommand:offset + startAddr data:writeData];
                }
                
                //组包后位置偏移
                offset += writeData.length;
                [weakSelf writeData:sendData peripheral:self.peripheral writeForChar:self.otaCharacter type:CBCharacteristicWriteWithoutResponse];
            }];
            
            //每包数据写完后的回调
            operation.completionBlock = ^{
                
                if (weakSelf.cancleUpdate) {
                    [weakSelf.writeQueue cancelAllOperations];
                    return;
                }
                
                
                if (nil != writeBlock) {
                    writeBlock(writeData.length);
                }
                
                //如果是最后一包，读取数据下发状态
                if (i == writeNum - 1) {

                    [NSThread sleepForTimeInterval:1];
                    if (isProgram) {
                        
                        weakSelf.programRespone = YES;
                        if (weakSelf.currentImageObject.chipType == ChipType_CH579) {
                            
                            
                        }else {
                            //读取编程结果
                            [weakSelf readData:weakSelf.peripheral readValueForChar:weakSelf.otaCharacter];
                            //等待结果读取

                            if (nil != progressBlock) {
                                progressBlock(progressType_ProgramWait);
                            }

                            for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
                                [NSThread sleepForTimeInterval:0.01];
                                if (weakSelf.readResponse) {
                                    break;
                                }
                            }

                            if (weakSelf.readResponse) {

                                NSData *valueData = [weakSelf.otaCharacter value];
                                Byte *response = (Byte *)[valueData bytes];
                                if (response != nil && valueData.length > 0 && response[0] == 0x00) {
                                    weakSelf.programRespone = YES;
                                    if (nil != progressBlock) {
                                        progressBlock(progressType_ProgramSuccess);
                                    }
                                }else {
                                    weakSelf.programRespone = NO;
                                    if (nil != progressBlock) {
                                        progressBlock(progressType_ProgramFailed);
                                    }
                                }
                            }else {
                                weakSelf.programRespone = NO;
                                if (nil != progressBlock) {
                                    progressBlock(progressType_ProgramFailed);
                                }
                            }

                            //读取操作复位
                            weakSelf.readResponse = YES;
                        }

                    }else {
                        
                        weakSelf.verifyRespone = YES;
                        //读取校验结果
                        [weakSelf readData:weakSelf.peripheral readValueForChar:weakSelf.otaCharacter];
                        //等待校验读取
                        if (nil != progressBlock) {
                            progressBlock(progressType_VerifyWait);
                        }
                        
                        for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
                            [NSThread sleepForTimeInterval:0.01];
                            if (weakSelf.readResponse) {
                                break;
                            }
                        }
                        
                        if (weakSelf.readResponse) {
                            NSData *valueData = [weakSelf.otaCharacter value];
                            Byte *response = (Byte *)[valueData bytes];
                            if (response != nil && valueData.length > 0 && response[0] == 0x00) {
                                weakSelf.verifyRespone = YES;
                                if (nil != progressBlock) {
                                    progressBlock(progressType_VerifySuccess);
                                }
                            }else {
                                weakSelf.verifyRespone = NO;
                                if (nil != progressBlock) {
                                    progressBlock(progressType_VerifyFailed);
                                }
                            }
                        }else {
                            weakSelf.verifyRespone = NO;
                            if (nil != progressBlock) {
                                progressBlock(progressType_VerifyFailed);
                            }
                        }
                        weakSelf.readResponse = YES;
                    }
                    dispatch_semaphore_signal(weakSelf.semaphore);
                }
                
                [weakSelf.writeQueue setSuspended:NO];
            };
            
            [weakSelf.writeQueue addOperation:operation];
        }
    }
}


#pragma mark -- Peripheral

- (void)startScanServiceForPeripheral:(CBPeripheral *)peripheral {
    //设置服务代理回调
    [peripheral setDelegate:self];
    [peripheral discoverServices:self.serviceUUIDS];
}

#pragma mark -- CBPeripheralDelegate

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
    NSString *log = [[NSString alloc] initWithFormat:@"发现服务:%@", peripheral.services];
    if (_isDebug) {
        NSLog(@"%@", log);
    }
    
    for (CBService *service in peripheral.services){
        [peripheral discoverCharacteristics:nil forService:service];
    }
    
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerPeriphearl:services:error:)]) {
        [self.delegate BLEManagerPeriphearl:peripheral services:peripheral.services error:error];
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {

    NSString *log = [[NSString alloc] initWithFormat:@"发现特征通道：%@", service.characteristics];
    if (_isDebug){
        NSLog(@"%@", log);
    }
    
    for (CBCharacteristic *characteristic in service.characteristics) {
        NSString *characterUUID = characteristic.UUID.UUIDString;
        if ([characterUUID isEqualToString:READ_WRITE_CHARACTER_UUID] && [service.UUID.UUIDString isEqualToString:SERVICE_UUID]) {
            self.otaCharacter = characteristic;
            _writeWithMaxLen = [peripheral maximumWriteValueLengthForType:CBCharacteristicWriteWithResponse];
            _writeWithoutMaxLen = [peripheral maximumWriteValueLengthForType:CBCharacteristicWriteWithoutResponse];
            log = [[NSString alloc] initWithFormat:@"特征通道写长度:%ld %ld",(long)_writeWithMaxLen, (long)_writeWithoutMaxLen];
            if (_isDebug){
                NSLog(@"%@", log);
            }
        
            [self sendImageCommand];
            [self peripheralNotify:peripheral setNotifyForChar:characteristic];
        }
    }
    
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerService:characteristics:error:)]) {
        [self.delegate BLEManagerService:service characteristics:service.characteristics error:error];
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {

    NSString *log = [[NSString alloc] initWithFormat:@"通知状态改变：%@", characteristic.isNotifying ? @"开启" : @"关闭"];
    if (_isDebug){
        NSLog(@"%@", log);
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {

    NSString *log = [[NSString alloc] initWithFormat:@"监听到了数据：%@， 特征：%@， error：%@", peripheral, characteristic, error];
    if (_isDebug){
        NSLog(@"%@", log);
    }
    
    self.readResponse = YES;
    
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerUpdateValueForCharacteristic:Characteristic:error:)]) {
        [self.delegate BLEManagerUpdateValueForCharacteristic:peripheral Characteristic:characteristic error:error];
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {

    NSString *log = [[NSString alloc] initWithFormat:@"写入数据：%@ 是否出错：%@", characteristic, error];
    if (_isDebug){
        NSLog(@"%@", log);
    }
    self.writeResponse = YES;
}

- (void)peripheralIsReadyToSendWriteWithoutResponse:(CBPeripheral *)peripheral{
    self.canSendWrite = [peripheral canSendWriteWithoutResponse];
    NSString *log = [[NSString alloc] initWithFormat:@"无响应写回调:IsReadyToSendWriteWithoutResponse"];
    if (_isDebug){
        NSLog(@"%@", log);
    }
}

#pragma mark - public func 读/写/订阅

- (void)cancleOTAUpdate {
    self.cancleUpdate = YES;
}

- (void)startOTAUpdate:(NSData *)data eraseAddr:(NSInteger)eraseAddr writeCallBack:(writeDataCallBack)writeBlock progressCallBack:(OTAProgressCallBack)progressBlock {
    
    if (self.currentImageObject == nil || self.peripheral == nil || self.otaCharacter == nil || data == nil) {
        return;
    }
    
    self.cancleUpdate = NO;
    NSInteger startAddr = 0;
    if (self.currentImageObject.chipType == ChipType_CH573 || self.currentImageObject.chipType == ChipType_CH583) {
        //CH583/CH573
        //BIN文件擦除地址由用户自己输入
        //HEX文件需要从调用
        startAddr = eraseAddr;
        
    }else if (self.currentImageObject.chipType == ChipType_CH579) {
        if (self.currentImageObject.type == ImageType_A) {
            startAddr = self.currentImageObject.offset;
        }else if (self.currentImageObject.type == ImageType_B) {
            startAddr = 0;
        }
    }
    
    NSInteger blockSize = self.currentImageObject.blockSize;
    NSInteger nBlocks = (data.length + (blockSize - 1)) / blockSize;
    
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        if (nil != progressBlock) {
            progressBlock(progressType_Erase);
        }
        
        //发送擦除命令
        [weakSelf sendEraseCmmand:startAddr block:nBlocks];
        
        //等待擦除信息返回
        for (int i = 0; i < TIMEOUT_REPEAT_NUM ; i++) {
            [NSThread sleepForTimeInterval:0.001];
            if (weakSelf.eraseResponse) {
                break;
            }
        }
      
        if (weakSelf.eraseResponse) {
            //擦除成功开始进行编程
            if (nil != progressBlock) {
                progressBlock(progressType_EraseSuccess);
            }
            [weakSelf udpataDataStream:data startAddr:startAddr writeCallBack:writeBlock progressCallBack:progressBlock isProgram:YES];
        }else {
            if (nil != progressBlock) {
                progressBlock(progressType_EraseFailed);
            }
            return;
        }
        //使用信号量等待更新完成
        if (weakSelf.semaphore) {
            dispatch_semaphore_wait(weakSelf.semaphore, DISPATCH_TIME_FOREVER);
        }
        
        //编程完成开始校验
        if (weakSelf.programRespone) {
            [weakSelf udpataDataStream:data startAddr:startAddr writeCallBack:writeBlock progressCallBack:progressBlock isProgram:NO];
        }else {
            if (nil != progressBlock) {
                progressBlock(progressType_ProgramFailed);
            }
            return;
        }
        //使用信号量等待校验完成
        if (weakSelf.semaphore) {
            dispatch_semaphore_wait(weakSelf.semaphore, DISPATCH_TIME_FOREVER);
        }
    
        //校验成功发送结束
        if (weakSelf.verifyRespone) {
            [weakSelf sendEndCmmand:progressBlock];
        }else {
            if (nil != progressBlock) {
                progressBlock(progressType_VerifyFailed);
            }
        }
    });
}

//发送数据
//@data 待发送数据
- (void)writeData:(NSData *)data peripheral:(CBPeripheral *)peripheral writeForChar:(CBCharacteristic *)characteristic type:(CBCharacteristicWriteType)type {
    
    if (nil == peripheral) {
        if (_isDebug) {
            NSLog(@"写时未获度外设对象");
        }
        return;
    }
    
    if (type == CBCharacteristicWriteWithResponse) {
        if (characteristic.properties & CBCharacteristicPropertyWrite) {
            self.writeResponse = NO;
            [peripheral writeValue:data forCharacteristic:characteristic type:type];
        }
    }
    
    if (type == CBCharacteristicWriteWithoutResponse) {
        if (characteristic.properties & CBCharacteristicPropertyWriteWithoutResponse) {
            self.canSendWrite = NO;
            [peripheral writeValue:data forCharacteristic:characteristic type:type];
        }
    }
}

//读取数据
- (void)readData:(CBPeripheral *)peripheral readValueForChar:(CBCharacteristic *)characteristic{
    if (characteristic.properties & CBCharacteristicPropertyRead) {
        self.readResponse = NO;
        [peripheral readValueForCharacteristic:characteristic];
    }
}

//订阅某个特征值
- (void)peripheralNotify:(CBPeripheral *)peripheral setNotifyForChar:(CBCharacteristic *)characteristic{
    if (characteristic.properties & CBCharacteristicPropertyNotify) {
        [peripheral setNotifyValue:YES forCharacteristic:characteristic];
    }
}

//取消某个订阅
- (void)peripheralNotify:(CBPeripheral *)peripheral cancleNotifyForChar:(CBCharacteristic *)characteristic{
    [peripheral setNotifyValue:NO forCharacteristic:characteristic];
}

#pragma mark - RSSI

- (void)readRSSI:(CBPeripheral *)peripheral rssiCallBack:(rssiCallBack)rssiBlock{
    [peripheral readRSSI];
    if (nil != rssiBlock) {
        self.rssiBlock = rssiBlock;
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didReadRSSI:(NSNumber *)RSSI error:(NSError *)error {
    if (self.rssiBlock != nil) {
        self.rssiBlock(RSSI);
    }
}

#pragma mark - CBCentralManagerDelegate
//蓝牙状态
- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
    [self BLEDidUpdateState:central.state];
}

//蓝牙状态
- (void)peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral {
    [self BLEDidUpdateState:peripheral.state];
}

-(void)BLEDidUpdateState:(CBManagerState)state{
    switch (state) {
        case CBManagerStatePoweredOn:
            // 扫描所有蓝牙设备
            [self startScan:nil options:nil];
            break;
        case CBManagerStateUnsupported:
            [self BLEManagerState:Manager_State_Unsupported];
            break;
        case CBManagerStatePoweredOff:
            [self BLEManagerState:Manager_State_PoweredOff];
            break;
        case CBManagerStateResetting:
            [self BLEManagerState:Manager_State_Resetting];
            break;
        case CBManagerStateUnauthorized:
            [self BLEManagerState:Manager_State_Unauthorized];
            break;
        default:
            [self BLEManagerState:Manager_State_Unknow];
            break;
    }
}

//扫描出错
-(void)BLEManagerState:(NSError*)error{
    if (self.delegate && [self.delegate respondsToSelector:@selector(BLEManagerDidUpdateState:)]) {
        [self.delegate BLEManagerDidUpdateState:error];
    }
}

//OTA字节对齐
- (NSInteger)getSpecificMaxPacketForOTAAlign:(NSInteger)mtu {
    NSInteger frameMaxLen = mtu;
    NSInteger dataMaxLen = frameMaxLen - 4;
    
    NSInteger dataCount = (dataMaxLen / 16) * 16;
    return dataCount + 4;
}

- (NSData *)hex2BinData:(NSData *)data {
    u_long size = data.length;
    Byte *biBuf = malloc(size * sizeof(Byte));
    [HexToBin HexToBin:(Byte *)[data bytes] hexLen:data.length binBuf:biBuf binLen:&size];
    NSData *binData = [[NSData alloc] initWithBytes:biBuf length:size];
    free(biBuf);
    return binData;
}

- (void)setChipType:(ChipType)type {
    self.currentImageObject.chipType = type;
    [BLEOTACmd updateAddressBase:type];
}

@end
