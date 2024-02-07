//
//  CBAdvertisementData.m
//  BLEOTALibrary
//
//  Created by moyun on 2022/4/7.
//

#import "CBAdvertisementData.h"
#import <CoreBluetooth/CoreBluetooth.h>

@implementation CBAdvertisementData

+ (NSString *)getAdvertisementDataName:(NSString *)key{
    if ([key isEqual: CBAdvertisementDataLocalNameKey]) {
        return @"Local Name";
    } else if ([key isEqual: CBAdvertisementDataTxPowerLevelKey]) {
        return @"Tx Power Level";
    } else if ([key isEqual: CBAdvertisementDataServiceUUIDsKey]) {
        return @"Service UUIDs";
    } else if ([key isEqual: CBAdvertisementDataServiceDataKey]) {
        return @"Service Data";
    } else if ([key isEqual: CBAdvertisementDataManufacturerDataKey]) {
        return @"Manufacturer Data";
    } else if ([key isEqual: CBAdvertisementDataOverflowServiceUUIDsKey]) {
        return @"Overflow Service UUIDs";
    } else if ([key isEqual: CBAdvertisementDataIsConnectable]){
        return @"Device is Connectable";
    } else if ([key isEqual: CBAdvertisementDataSolicitedServiceUUIDsKey]) {
        return @"Solicited Service UUIDs";
    }
    return key;
}

+ (NSString *)getAdvertisementDataStringValue:(NSDictionary *)datas Key:(NSString *)key{
    NSString *resultString  = @"";
    if ([key isEqual:CBAdvertisementDataLocalNameKey]) {
        resultString = datas[CBAdvertisementDataLocalNameKey];
    }
    else if ([key isEqual: CBAdvertisementDataTxPowerLevelKey]) {
        resultString = datas[CBAdvertisementDataTxPowerLevelKey];
    }
    else if ([key isEqual: CBAdvertisementDataServiceUUIDsKey]) {
        NSArray *serviceUUIDs = datas[CBAdvertisementDataServiceUUIDsKey];
            if (serviceUUIDs == nil) {
                return @"";
            }
        for (int i = 0; i < [serviceUUIDs count]; i++){
            resultString = [NSString stringWithFormat:@"%@%@,",resultString,[serviceUUIDs objectAtIndex:i]];
        }
        resultString = [resultString substringToIndex:[resultString length]-1];;
    }
    else if ([key isEqual: CBAdvertisementDataServiceDataKey]) {
        NSDictionary *data = datas[CBAdvertisementDataServiceDataKey];
        if (data == nil) {
            return @"";
        }
        NSLog(@"%@",data);
        resultString = [[[data description] stringByReplacingOccurrencesOfString:@"\n" withString:@""] stringByReplacingOccurrencesOfString:@" " withString:@""];
    }
    else if ([key isEqual: CBAdvertisementDataManufacturerDataKey]) {
        NSData *data = datas[CBAdvertisementDataManufacturerDataKey];
        resultString = [NSString stringWithFormat:@"%lu bytes",(unsigned long)data.length];
    } else if ([key isEqual: CBAdvertisementDataOverflowServiceUUIDsKey]) {
        resultString = @"";
    } else if ([key isEqual: CBAdvertisementDataIsConnectable]){
        NSNumber *connectable = datas[key];
        if (connectable){
            resultString = connectable.boolValue ? @"true" : @"false";
        }
    } else if ([key isEqual: CBAdvertisementDataSolicitedServiceUUIDsKey]) {
        resultString = @"";
    }
    if (resultString == nil) {
        resultString = @"";
    }
    return resultString;
}

@end
