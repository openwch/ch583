//
//  CharacteristicsTableViewCell.swift
//  WCHBLEApp
//
//  Created by 娟华 胡 on 2021/3/23.
//

import UIKit
import CoreBluetooth

class CH_CharacteristicsTableViewCell: UITableViewCell {
    
    @IBOutlet weak var nameLabel: UILabel!
    @IBOutlet weak var uuidLabel: UILabel!
    @IBOutlet weak var propertiesLabel: UILabel!
    
    @IBOutlet weak var receiveButton: UIButton!
    @IBOutlet weak var sendButton: UIButton!
    
    @IBOutlet weak var sendImageView: UIImageView!
    @IBOutlet weak var receiveImageView: UIImageView!
    
    public var characterSend:(()->Void)?
    public var characterReceive:(()->Void)?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
        self.receiveButton.isHidden = true
        self.sendButton.isHidden = true
    }

    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)

        // Configure the view for the selected state
    }
    
    @IBAction func showSendView(_ sender: Any) {
        self.characterSend?()
    }
    
    @IBAction func showReadView(_ sender: Any) {
        self.characterReceive?()
    }
    
    public func updateCharactertics(character:CBCharacteristic) {
        self.nameLabel.text = "\(character.uuid)"
        self.uuidLabel.text = "UUID:\(character.uuid.uuidString)"
        
        var properties = "properties:"
        if (character.properties.contains(.read)) {
            properties += " Read"
        }
        
        if (character.properties.contains(.write) || character.properties.contains(.writeWithoutResponse)) {
            properties += " Write"
        }
        
        if (character.properties.contains(.notify)) {
            properties += " Notify"
        }
        
        self.propertiesLabel.text = properties
        
        if (character.properties.contains(.read) || character.properties.contains(.notify)) {
            self.receiveButton.isHidden = false
            self.receiveImageView.isHidden = false
            
        }else {
            self.receiveButton.isHidden = true
            self.receiveImageView.isHidden = true
        }
        
        if (character.properties.contains(.write)) {
            self.sendButton.isHidden = false
            self.sendImageView.isHidden = false
        }else {
            self.sendButton.isHidden = true
            self.sendImageView.isHidden = true
        }
        
    }
    
}
