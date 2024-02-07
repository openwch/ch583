//
//  CH_PeripheralModel.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/11.
//

import UIKit
import CoreBluetooth

class CH_PeripheralModel: NSObject {
    let peripheral:CBPeripheral
    var RSSI:NSNumber = 0
    var advertisementData:[String:Any] = [:]
    var lastUpdatedTimeInterval: TimeInterval
    
    init(_ peripheral: CBPeripheral) {
        self.peripheral = peripheral
        self.lastUpdatedTimeInterval = Date().timeIntervalSince1970
    }
}
