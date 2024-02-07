//
//  CH_PeripheralSerivceViewController.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/12.
//

import UIKit
import XappKit
import BLEOTALibrary
import CoreBluetooth

class CH_PeripheralSerivceViewController: MH_VC_BaseViewController {
    
    public var serviceBackAction:(()->Void)?
    public var services:[CBService] = [CBService]()
    private var serviceCharacter:[String:[CBCharacteristic]] = [String:[CBCharacteristic]]()

    @IBOutlet weak var tableView: UITableView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.title = "Device Information"
        _=self.leftBarItem(with: UIImage.init(named: "btn_common_back"), selector: #selector(backAction));
        _=self.rightBarItem(with: "Update", selector: #selector(updateAction))
        // Do any additional setup after loading the view.
        self.setupViews()

    }
    
    override func backAction() {
        self.serviceBackAction?()
        super.backAction()
    }

    func setupViews() {
        self.tableView.tableFooterView = UIView()
        self.tableView.estimatedRowHeight = 0
        self.tableView.estimatedSectionHeaderHeight = 0
        self.tableView.estimatedSectionFooterHeight = 0
    }
    
    public func servicesCharacterList(list:[String:[CBCharacteristic]]) {
        self.serviceCharacter = list
        self.tableView.reloadData()
    }
    
    @objc
    private func updateAction() {
        
        let object:CurrentImageObject? = BLEOTAManager.shareInstance().getCurrentImageInfo();
        if (object == nil) {
            _ = self.showFailedMessage("未获取到固件信息，请重新连接")
        }else {
            let fileVC = CH_FileSelectorViewController<File>.init(documentPicker: WCHDocumentPicker())
            self.navigationController?.pushViewController(fileVC, animated: true)
        }
    }
}

extension CH_PeripheralSerivceViewController:UITableViewDelegate, UITableViewDataSource {
    
    func numberOfSections(in tableView: UITableView) -> Int {
        return services.count
    }
    
    func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        return 70
    }
    
    func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let headView = CH_ServiceTableHeadView.instanceFromNib()
        if (services.count > section) {
            let service = services[section]
            headView?.updateServiceHead(service: service)
        }
        
        return headView
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 86
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        if (services.count > section) {
            let service = services[section]
            let uuid = service.uuid.uuidString
            let characters = self.serviceCharacter[uuid]
            return characters?.count ?? 0
        }
        return 0
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        var cell = tableView.dequeueReusableCell(withIdentifier: "CH_CharacteristicsTableViewCell") as? CH_CharacteristicsTableViewCell
        if (cell == nil) {
            cell = CH_CharacteristicsTableViewCell.instanceFromNib()
        }
        cell?.selectionStyle = .none
        if (self.services.count > indexPath.section) {
            let service = self.services[indexPath.section]
            let characters = self.serviceCharacter[service.uuid.uuidString]
            if ((characters?.count ?? 0) > indexPath.row) {
                cell?.updateCharactertics(character: characters![indexPath.row])
            }
            
            cell?.characterSend = {[weak self] in
                guard let wself = self else { return }
            }
            
            cell?.characterReceive =  {[weak self] in
                guard let wself = self else { return }
            }
        }
        return cell ?? UITableViewCell()
    }
    
}
