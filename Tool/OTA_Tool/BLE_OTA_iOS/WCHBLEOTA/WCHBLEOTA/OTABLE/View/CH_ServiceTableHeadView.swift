//
//  ServiceTableHeadView.swift
//  WCHBLEApp
//
//  Created by 娟华 胡 on 2021/3/23.
//

import UIKit
import CoreBluetooth


class CH_ServiceTableHeadView: UIView {

    @IBOutlet weak var nameLabel: UILabel!
    @IBOutlet weak var uuidLabel: UILabel!
    @IBOutlet weak var privateLabel: UILabel!
    
    public func updateServiceHead(service:CBService) {
        self.nameLabel.text = "\(service.uuid)"
        self.uuidLabel.text = "UUID:" + service.uuid.uuidString
        self.privateLabel.text = service.isPrimary ? "PRIMARY SERVICE" : "NO PRIMIARY"
    }

}
