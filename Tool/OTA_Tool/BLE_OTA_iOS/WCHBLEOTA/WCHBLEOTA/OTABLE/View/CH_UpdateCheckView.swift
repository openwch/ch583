//
//  CH_UpdateCheckView.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/25.
//

import UIKit
import BLEOTALibrary

class CH_UpdateCheckView: UIView {
    
    @IBOutlet weak var chipTypeLabel: UILabel!
    
    @IBOutlet weak var imageTypeLabel: UILabel!
    
    @IBOutlet weak var blockSizeLabel: UILabel!
    
    @IBOutlet weak var offsetLabel: UILabel!
    
    @IBOutlet weak var containView: UIView!
    
    @IBOutlet weak var segmentView: UISegmentedControl!
    
    
    var chipTypeSelect:((NSInteger)->Void)?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        
        self.containView.layer.cornerRadius = 15
        self.containView.layer.masksToBounds = true
    }
    
    @IBAction func segMentSelect(_ sender: Any) {
        
        let seg:UISegmentedControl = sender as! UISegmentedControl
        
        self.chipTypeSelect?(seg.selectedSegmentIndex)
    }
    
    public func updateCheckInfo(model:CurrentImageObject) {
        
        var chipType:String = ""
        var imageType:String = ""
        if (model.chipType == .CH573) {
            chipType = "CH573"
        }else if (model.chipType == .CH583) {
            chipType = "CH583"
        }else if (model.chipType == .CH579) {
            chipType = "CH579"
        }else {
            chipType = "UNKNOW"
        }
        
        if (model.type == .A) {
            imageType = "ImageA"
        }else if (model.type == .B) {
            imageType = "ImageB"
        }else {
            imageType = "UNKNOW"
        }
        
        self.chipTypeLabel.text = chipType
        self.imageTypeLabel.text = imageType
        self.blockSizeLabel.text = "\(model.blockSize)"
        self.offsetLabel.text = "\(model.offset)"
        
        if (chipType != "UNKNOW") {
            self.segmentView.isHidden = true
        }
    }
    
    @IBAction func backGroundAction(_ sender: Any) {
        self.removeFromSuperview()
    }
    
}
