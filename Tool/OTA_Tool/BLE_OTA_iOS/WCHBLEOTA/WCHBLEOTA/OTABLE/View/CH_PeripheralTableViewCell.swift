//
//  PeripheralTableViewCell.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/6.
//

import UIKit
import CoreBluetooth

class CH_PeripheralTableViewCell: UITableViewCell {
    
    @IBOutlet weak var signalStrengthImg: UIImageView!
    @IBOutlet weak var nameLabel: UILabel!
    @IBOutlet weak var rssiLabel: UILabel!
    @IBOutlet weak var stateLabel: UILabel!
    @IBOutlet weak var identifierLabel: UILabel!
    @IBOutlet weak var connectBtn: UIButton!
    
    public var peripheralConnect:((CBPeripheral?)->Void)?
    private var peripheral:CBPeripheral?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
    }
    
    public class func loadCell() -> CH_PeripheralTableViewCell?{
        let bundle = Bundle.init(for: CH_PeripheralTableViewCell.self)
        let nibs = bundle.loadNibNamed("CH_PeripheralTableViewCell", owner: self, options: nil)
        return nibs?.first as? CH_PeripheralTableViewCell
    }
    
    @IBAction func connectAction(_ sender: Any) {
        self.peripheralConnect?(self.peripheral)
    }
    
    public func updatePeripheralCell(model:CH_PeripheralModel) {
        self.peripheral = model.peripheral
        if ((model.peripheral.name?.isEmpty) != nil) {
            self.nameLabel.text = (model.peripheral.name ?? "")
        }else {
            self.nameLabel.text = "Unnamed"
        }

        self.rssiLabel.text = String.init(format: "%d", model.RSSI as? Int ?? 0)
        self.identifierLabel.text = "UUID:\(model.peripheral.identifier)"
        if let serviceUUIDs = model.advertisementData["kCBAdvDataServiceUUIDs"] as? NSArray, serviceUUIDs.count != 0 {
            self.stateLabel.text = "\(serviceUUIDs.count) service" + (serviceUUIDs.count > 1 ? "s" : "")
        } else {
            self.stateLabel.text = "No service"
        }
        
        switch labs(model.RSSI.intValue) {
        case 0...40:
            self.signalStrengthImg.image = UIImage(named: "signal_strength_5")
        case 41...53:
            self.signalStrengthImg.image = UIImage(named: "signal_strength_4")
        case 54...65:
            self.signalStrengthImg.image = UIImage(named: "signal_strength_3")
        case 66...77:
            self.signalStrengthImg.image = UIImage(named: "signal_strength_2")
        case 77...89:
            self.signalStrengthImg.image = UIImage(named: "signal_strength_1")
        default:
            self.signalStrengthImg.image = UIImage(named: "signal_strength_0")
        }
    }
}

