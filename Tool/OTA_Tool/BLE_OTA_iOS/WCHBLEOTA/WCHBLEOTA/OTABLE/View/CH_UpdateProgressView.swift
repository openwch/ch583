//
//  CH_UpdateProgressView.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/24.
//

import UIKit

class CH_UpdateProgressView: UIView {
    
    enum Style {
        case update, error, done
    }
    
    @IBOutlet var updateLogoImage: UIImageView!
    @IBOutlet var progressView: UIProgressView!
    @IBOutlet var statusLabel: UILabel!
    
    var style: Style = .update {
        didSet {
            let colorImage: (UIColor, ModernIcon, UIImage?) = {
                let oldImage = UIImage(named: "update_ic")?.withRenderingMode(.alwaysTemplate)
                switch style {
                case .done: return (UIColor(named: "WCHGreen")!, ModernIcon.checkmark(.circle), oldImage)
                case .error: return (UIColor(named: "WCHRed")!, ModernIcon.exclamationmark(.triangle), oldImage)
                case .update: return (UIColor(named: "WCHBlue")!, ModernIcon.arrow(.init(digit: 2))(.circlePath), oldImage)
                }
            }()
            
            updateLogoImage.tintColor = colorImage.0
            if #available(iOS 13, *) {
                updateLogoImage.image = colorImage.1.image
            } else {
                updateLogoImage.image = colorImage.2
            }
            
            progressView.tintColor = colorImage.0
            statusLabel.textColor = style == .error ? UIColor(named: "WCHRed")! : UIColor.black
            
            if style != .update {
                stopAnimating()
            }
        }
    }
    
    func startAnimating() {
//        updateLogoImage.translatesAutoresizingMaskIntoConstraints = true
        
        UIView.animateKeyframes(withDuration: 1, delay: 0, options: [.repeat], animations: {
            UIView.addKeyframe(withRelativeStartTime: 0, relativeDuration: 1) {
                self.updateLogoImage.transform = CGAffineTransform(rotationAngle: .pi)
            }
            UIView.addKeyframe(withRelativeStartTime: 0, relativeDuration: 0.5) {
                self.updateLogoImage.transform = CGAffineTransform(scaleX: 1.2, y: 1.2)
            }
            UIView.addKeyframe(withRelativeStartTime: 0.5, relativeDuration: 0.5) {
                self.updateLogoImage.transform = CGAffineTransform(scaleX: 1.0, y: 1.0)
            }
        })
    }
    
    func stopAnimating() {
        updateLogoImage.layer.removeAllAnimations()
    }
    
    public func updateStatus(status:String) {
        self.statusLabel.text = status
    }
    
    public func updateProgress(progress:Float) {
        self.progressView.progress = progress
    }
}
