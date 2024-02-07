//
//  CH_FileTableViewCell.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import UIKit

class CH_FileTableViewCell: UITableViewCell {

    @IBOutlet var nameLabel: UILabel!
    @IBOutlet var fileImage: UIImageView!
    @IBOutlet var infoLabel: UILabel!
    @IBOutlet var dateLabel: UILabel!
    @IBOutlet var leadingConstraint: NSLayoutConstraint!
    
    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
    }

    
    func update(_ model: FileNodeRepresentation) {
        nameLabel.text = model.name
        fileImage.image = model.image
        infoLabel.text = model.sizeInfo
        
        let dafeFormatter = DateFormatter()
        dafeFormatter.timeStyle = .none
        dafeFormatter.dateStyle = .short
        dateLabel.text = model.modificationDate.flatMap { dafeFormatter.string(from: $0) }
        
        leadingConstraint.constant = CGFloat(model.level * 12)
        layoutIfNeeded()
        
        if model.node is Directory {
            selectionStyle = .none
        } else {
            selectionStyle = .default
        }
        
        fileImage.alpha = model.valid ? 1 : 0.5
        
    }
}
