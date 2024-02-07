//
//  CH_HomeViewController.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/6.
//

import UIKit
import XappKit
import BLEOTALibrary

class CH_HomeViewController: MH_VC_BaseViewController {

    @IBOutlet weak var tableView: MH_V_RefreshTableView!
    
    var preferences: CH_Preferences? {
        return CH_PreferencesStore.shared.preferences
    }
    
    private var OTAManager:BLEOTAManager?
    private var connectPeripheral:CBPeripheral?
    private var devicesArray:[CH_PeripheralModel] = [CH_PeripheralModel]()

    private var servicesArray:[CBService] = [CBService]()
    private var serviceCharacter:[String:[CBCharacteristic]] = [String:[CBCharacteristic]]()
    private weak var serviceVC:CH_PeripheralSerivceViewController?
    private var writeCallBack:writeDataCallBack?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.navbarStatus = .Show
        self.title = "Device List"
        _=self.rightBarItem(with: "Filter", selector: #selector(filterAction))
        
        // Do any additional setup after loading the view.
        self.setupHome()
    }

    func setupHome() {
        self.OTAManager = BLEOTAManager.shareInstance()
        self.OTAManager?.delegate = self
        
        self.tableView.tableFooterView = UIView()
        self.tableView.setTableViewRefreshHeader()
        self.tableView.refreshHeaderHandler = { [weak self] in
            guard let wself = self else { return }
            wself.devicesArray.removeAll()
            wself.scanAllPeripheral()
        }
    }
    
    private func scanAllPeripheral() {
        self.devicesArray.removeAll()
        self.tableView.reloadData()
        self.OTAManager?.stopScan()
        self.OTAManager?.startScan(nil)
    }
    
    private func stopScan() {
        self.OTAManager?.stopScan()
    }
    
    private func setConnectSuccess() {
        let sVC = CH_PeripheralSerivceViewController.init(nibName: "CH_PeripheralSerivceViewController", bundle: Bundle.init(for: CH_PeripheralSerivceViewController.self))
        
        sVC.services = self.servicesArray

        sVC.serviceBackAction = { [weak self] in
            guard let wself = self else { return }
            wself.OTAManager?.disconnect(wself.connectPeripheral)
            wself.scanAllPeripheral()
        }
        
        self.navigationController?.pushViewController(sVC, animated: true)
        self.serviceVC = sVC
    }
    
    @objc func filterAction() {
        let filterVC = CH_FilterViewController.init(nibName: "CH_FilterViewController", bundle: Bundle.init(for: CH_FilterViewController.self))
        filterVC.filterCallBack = { [weak self] in
            guard let wself = self else { return }
            wself.scanAllPeripheral()
        }
        filterVC.sortedCallBack = { [weak self] in
            guard let wself = self else { return }
            wself.devicesArray = wself.devicesArray.sorted {
                $0.RSSI.intValue > $1.RSSI.intValue
            }
            wself.tableView.reloadData()
        }
        self.navigationController?.pushViewController(filterVC, animated: true)
    }
    
    @IBAction func scanFileAction(_ sender: Any) {
         //获取目录下的文件
//         let documentPath = NSHomeDirectory() + "/Documents/all.hex"
//         print(documentPath)
//
//         //判断文件是否存在
//         if FileManager.default.fileExists(atPath: documentPath) {
//             print("文件存在");
 //            let fileUrl = URL.init(fileURLWithPath: documentPath)
 //            let data = try? Data(contentsOf: fileUrl)
 //
 //            print(data);
//         }else {
//
//         }
        let fileVC = CH_FileSelectorViewController<File>.init(documentPicker: WCHDocumentPicker())
        self.navigationController?.pushViewController(fileVC, animated: true)

    }
}

extension CH_HomeViewController:UITableViewDelegate, UITableViewDataSource {
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 100
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return self.devicesArray.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        var cell = tableView.dequeueReusableCell(withIdentifier: "CH_PeripheralTableViewCell") as? CH_PeripheralTableViewCell
        if (cell == nil) {
            cell = CH_PeripheralTableViewCell.loadCell()
        }
        
        cell?.selectionStyle = .none
        cell?.peripheralConnect = {[weak self] (perpheral) in
            guard let wself = self else { return }
            if (perpheral == nil) {
                _ = wself.showFailedMessage("为获取外设对象")
                return
            }
            if (perpheral?.state == .connected) {
                wself.OTAManager?.disconnect(perpheral)
            }else {
                wself.OTAManager?.connect(perpheral)
                _ = wself.showLoadingWithText("正在连接...")
            }
        }
        
        if (self.devicesArray.count > indexPath.row) {
            let model = self.devicesArray[indexPath.row]
            cell?.updatePeripheralCell(model:model)
        }
        
        return cell ?? UITableViewCell()
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {

    }
    
}

extension CH_HomeViewController:BLEOTADelegate {
    
    func bleManagerDidDiscover(_ peripheral: CBPeripheral!, advertisementData: [String : Any]!, rssi RSSI: NSNumber!) {
        if (peripheral == nil || advertisementData == nil || RSSI == nil) {
            return
        }
        self.tableView.endHeaderRefersh()
        let peripheralInfo = CH_PeripheralModel.init(peripheral)
        if (!self.devicesArray.contains(peripheralInfo)) {
            let nowTimeInterval = Date().timeIntervalSince1970
            
            if let preference = preferences, preference.needFilter {
                if RSSI!.intValue != 127, RSSI!.intValue > preference.filter {
                    peripheralInfo.advertisementData = advertisementData!
                    peripheralInfo.RSSI = RSSI!
                    peripheralInfo.lastUpdatedTimeInterval = nowTimeInterval
                    self.devicesArray.append(peripheralInfo)
                }
            } else {
                peripheralInfo.advertisementData = advertisementData!
                peripheralInfo.RSSI = RSSI!
                peripheralInfo.lastUpdatedTimeInterval = nowTimeInterval
                self.devicesArray.append(peripheralInfo)
            }

        }else {
            guard let index = self.devicesArray.firstIndex(of: peripheralInfo) else {
                return
            }
            let originPeripheralInfo = self.devicesArray[index]
            let nowTimeInterval = Date().timeIntervalSince1970
            
            originPeripheralInfo.lastUpdatedTimeInterval = nowTimeInterval
            originPeripheralInfo.RSSI = RSSI!
            originPeripheralInfo.advertisementData = advertisementData!
        }
        self.tableView.reloadData()
    }
    
    func bleManagerDidUpdateState(_ error: Error?) {
        let nsError = error as NSError?
        let errorInfo = nsError?.userInfo
        print(errorInfo?["NSUnderlyingError"] as? String ?? "未知错误")
        _=self.showFailedMessage(errorInfo?["NSUnderlyingError"] as? String ?? "未知错误")
    }
    
    func bleManagerDidPeripheralConnectUpateState(_ peripheral: CBPeripheral!, error: Error?) {
        self.hideHud()
        if (nil == error) {
            _=self.showSuccessMessage("连接成功")
            self.connectPeripheral = peripheral
      
            self.OTAManager?.readRSSI(peripheral, rssiCallBack: { (number) in
                print("读取到了信号强度",Int(truncating: number ?? 0))
            })
        }else {
            let nsError = error as NSError?
            let errorInfo = nsError?.userInfo
            _=self.showFailedMessage(errorInfo?["NSUnderlyingError"] as? String ?? "未知错误")
        }
        self.tableView.reloadData()
    }

    func bleManagerPeriphearl(_ peripheral: CBPeripheral!, services: [CBService]?, error: Error?) {
        if (nil == error) {
            self.servicesArray.removeAll()
            self.servicesArray = services ?? [CBService]()
            self.setConnectSuccess()
        }
    }
    
    func bleManagerService(_ service: CBService!, characteristics: [CBCharacteristic]!, error: Error?) {
        if (nil == error) {
            self.serviceCharacter[service?.uuid.uuidString ?? ""] = characteristics
            self.serviceVC?.servicesCharacterList(list: self.serviceCharacter)
        }
    }
    
    func bleManagerUpdateValue(forCharacteristic peripheral: CBPeripheral!, characteristic: CBCharacteristic!, error: Error?) {
        
    }
}
