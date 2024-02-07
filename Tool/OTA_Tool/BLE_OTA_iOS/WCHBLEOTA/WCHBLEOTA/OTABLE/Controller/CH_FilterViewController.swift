//
//  FilterViewController.swift
//  WCHBLEApp
//
//  Created by 娟华 胡 on 2021/4/2.
//

import UIKit
import XappKit

class CH_FilterViewController: MH_VC_BaseViewController {

    @IBOutlet var needFilterLayoutConstraints: [NSLayoutConstraint]!
    @IBOutlet var notNeedFilterLayoutConstraints: [NSLayoutConstraint]!
    @IBOutlet weak var needFilterSwitch: UISwitch!
    @IBOutlet weak var filterValueSlider: UISlider!
    @IBOutlet weak var minimumRSSILabel: UILabel!
    
    public var filterCallBack:(()->Void)?
    public var sortedCallBack:(()->Void)?
    
    var needFilter: Bool {
        get {
            return CH_PreferencesStore.shared.preferences?.needFilter ?? false
        }
        set(newValue) {
            guard let _ = CH_PreferencesStore.shared.preferences else {
                CH_PreferencesStore.shared.preferences = CH_Preferences()
                CH_PreferencesStore.shared.preferences!.needFilter = newValue
                return
            }
            CH_PreferencesStore.shared.preferences!.needFilter = newValue
        }
    }
    var filterValue: Int {
        get {
            return CH_PreferencesStore.shared.preferences?.filter ?? -90
        }
        set(newValue) {
            guard let _ = CH_PreferencesStore.shared.preferences else {
                CH_PreferencesStore.shared.preferences = CH_Preferences()
                CH_PreferencesStore.shared.preferences!.needFilter = true
                CH_PreferencesStore.shared.preferences!.filter = newValue
                return
            }
            CH_PreferencesStore.shared.preferences!.filter = newValue
        }
    }
    
    override func viewDidLoad() {
        _=self.leftBarItem(with: UIImage.init(named: "btn_common_back"), selector: #selector(backAction));
        _=self.rightBarItem(with: "Sort", selector: #selector(sortAction))
        super.viewDidLoad()
        initAll()
    }
    
    override func viewWillLayoutSubviews() {
        super.viewWillLayoutSubviews()
        if needFilterSwitch.isOn {
            NSLayoutConstraint.deactivate(self.notNeedFilterLayoutConstraints)
            NSLayoutConstraint.activate(self.needFilterLayoutConstraints)
        } else {
            NSLayoutConstraint.deactivate(self.needFilterLayoutConstraints)
            NSLayoutConstraint.activate(self.notNeedFilterLayoutConstraints)
        }
    }
    
    private func initAll() {
        self.toolbarItems = self.navigationController?.toolbar.items
        if needFilter {
            needFilterSwitch.isOn = true
            filterValueSlider.value = Float(filterValue)
        } else {
            needFilterSwitch.isOn = false
        }
        reloadViews()
    }
    
    private func reloadViews() {
        if needFilterSwitch.isOn {
            NSLayoutConstraint.deactivate(self.notNeedFilterLayoutConstraints)
            NSLayoutConstraint.activate(self.needFilterLayoutConstraints)
            
            let minimumValue = abs(Int(filterValue))
            minimumRSSILabel.text =  "Minimum RSSI: -\(minimumValue) dB"
//            var barValue = 0
//            switch labs(minimumValue) {
//            case 0...40:
//                barValue = 5
//            case 41...53:
//                barValue = 4
//            case 54...65:
//                barValue = 3
//            case 66...77:
//                barValue = 2
//            case 77...89:
//                barValue = 1
//            default:
//                barValue = 0
//            }
//            minimumRSSILabel.text =  "Minimum RSSI: -\(minimumValue) dB (\(barValue) bar\(barValue <= 1 ? "" : "s"))"
        } else {
            NSLayoutConstraint.deactivate(self.needFilterLayoutConstraints)
            NSLayoutConstraint.activate(self.notNeedFilterLayoutConstraints)
        }
    }
    
    @IBAction func didSwitchToggle(_ sw: UISwitch) {
        needFilter = sw.isOn
        reloadViews()
    }
    
    @IBAction func didSliderChang(_ slider: UISlider) {
        filterValue = Int(slider.value)
        self.filterCallBack?()
        reloadViews()
    }
    
    @objc func sortAction() {
        self.sortedCallBack?()
        _=self.showSuccessMessage("排序成功")
    }


}
