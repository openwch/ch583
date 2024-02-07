//
//  ImageWrapper.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import UIKit.UIImage


struct ImageWrapper {
    var modernIcon: ModernIcon
    var legacyIcon: UIImage?
    
    var image: UIImage? {
        if #available(iOS 13, *) {
            return modernIcon.image
        } else {
            return legacyIcon
        }
    }
    
    init(icon: ModernIcon, image: UIImage?) {
        modernIcon = icon
        legacyIcon = image
    }
    
    init(icon: ModernIcon, imageName: String) {
        modernIcon = icon
        legacyIcon = UIImage(named: imageName)
    }
}
