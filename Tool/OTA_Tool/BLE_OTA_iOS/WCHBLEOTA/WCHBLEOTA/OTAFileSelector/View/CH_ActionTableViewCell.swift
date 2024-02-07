//
//  CH_ActionTableViewCell.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/11.
//

import UIKit

class CH_ActionTableViewCell: UITableViewCell {


    enum Style {
        case `default`, destructive
    }
    
    var style: Style = .default {
        didSet {
            switch style {
            case .default:
                textLabel?.textColor = UIColor.blue
            case .destructive:
                textLabel?.textColor = UIColor.red
            }
        }
    }
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupAppearance()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
        setupAppearance()
    }

    private func setupAppearance() {
        let defaultSize = textLabel?.font.pointSize ?? 12
        textLabel?.font = UIFont.systemFont(ofSize: defaultSize)
        
        switch style {
        case .default:
            textLabel?.textColor = .blue
        case .destructive:
            textLabel?.textColor = .red
        }
    }

}
