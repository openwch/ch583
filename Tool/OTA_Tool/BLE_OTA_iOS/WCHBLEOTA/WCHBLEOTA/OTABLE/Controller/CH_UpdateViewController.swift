//
//  CH_UpdateViewController.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/14.
//

import UIKit
import XappKit
import SnapKit
import BLEOTALibrary

class CH_UpdateViewController: MH_VC_BaseViewController {

    private var progressView: CH_UpdateProgressView?
    private var checkView:CH_UpdateCheckView?
    private var logView:CH_LogView?
    
    @IBOutlet weak var inputTextField: UITextField!
    
    @IBOutlet weak var progressContainView: UIView!
    
    @IBOutlet weak var logContainView: UIView!
    
    public var fname:String?
    
    public var fileData:Data?
    
    public var ishex:Bool?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.title = "Update"
        _ = self.leftBarItem(with: UIImage.init(named: "btn_common_back"), selector: #selector(backAction))
        _ = self.rightBarItem(with: "Check", selector: #selector(checkAction))
        // Do any additional setup after loading the view.
        
        if (nil == self.progressView) {
            self.progressView = CH_UpdateProgressView.instanceFromNib()
        }
        self.progressContainView.addSubview(self.progressView!)
        self.progressView!.snp.makeConstraints { make in
            make.left.equalTo(0)
            make.right.equalTo(0)
            make.top.equalTo(0)
            make.bottom.equalTo(0)
        }
    }
    
    @IBAction func cancleUpdateAction(_ sender: Any) {
        
        self.progressView?.stopAnimating()
        self.progressView?.style = .error
        self.progressView?.updateStatus(status: "升级已取消")
        self.progressView?.updateProgress(progress: 0)
        
        BLEOTAManager.shareInstance().cancleOTAUpdate();
    }
    

    @IBAction func startUpdateAction(_ sender: Any) {
        
        var inputStr = self.inputTextField.text ?? "0"
        
        if (!inputStr.isHexNumber) {
            _ = self.showFailedMessage("Invalid Hex Number")
            return;
        }
        
        if (self.fileData == nil) {
            return;
        }
        
        let model = BLEOTAManager.shareInstance().getCurrentImageInfo()
        if (model?.chipType == .UNKNOW) {
            _ = self.showFailedMessage("Please Check Chip Type")
        }
        
        
        if inputStr.count == 0 {
            inputStr = "0";
        }
        
        let addr = Int(inputStr, radix: 16)!
        var startTime = Date.init(timeIntervalSinceNow: 0).timeIntervalSince1970
        var updateCount = 0;

        var sendData:Data? = self.fileData

        //根据数据格式，设定源数据文件是否需要进行偏移
        //hex格式已经转化bin后，已经对源数据文件进行了偏移，所以只需设置bin文件数据位置
        if (!self.ishex!) {
            let data = sendData?.subdata(in: addr ..< sendData!.count)
            print(data!)
            sendData = data
        }
        
        let totalLength:Int = sendData!.count * 2;
        var receiveLength = 0;
        
        
        self.progressView?.startAnimating()
        self.progressView?.style = .update
        
        BLEOTAManager.shareInstance().startOTAUpdate(sendData!, eraseAddr: addr) {[weak self] (NSInteger) in
            guard let sself = self else {return}
            receiveLength += NSInteger
            updateCount += NSInteger
        
            DispatchQueue.main.async {
                let value = Float(receiveLength) / Float(totalLength)
                sself.progressView?.updateProgress(progress: value)
//                let currentTime = Date.init(timeIntervalSinceNow: 0).timeIntervalSince1970
//                let time = currentTime - (startTime )
//                if time > 0 {
//                    let speed = Double(updateCount) / time
//                    if (speed > 1000) {
//                        sself.progressView.text = String.init(format: "接收速度：%0.1f KB/S", speed / 1024)
//                    }else {
//                        sself.progressView.text = String.init(format: "接收速度：%0.1f B/S", speed)
//                    }
//                }
            }
        } progressCallBack: {[weak self] (type) in
            guard let sself = self else {return}
        
            var statusStr:String = "Connecting"
            DispatchQueue.main.async {
                switch type {
                case .erase:
                    statusStr = "开始擦除"
                    break
                case .eraseSuccess:
                    statusStr = "擦除成功"
                    break
                case .eraseFailed:
                    statusStr = "擦除失败"
                    sself.progressView?.stopAnimating()
                    sself.progressView?.style = .error
                    break
                case .program:
                    statusStr = "开始数据写入"
                    break
                case .programWait:
                    statusStr = "等待数据发送结果"
                    break
                case .programSuccess:
                    statusStr = "数据写入成功"
                    startTime = Date.init(timeIntervalSinceNow: 0).timeIntervalSince1970
                    updateCount = 0;
                    break
                case .programFailed:
                    statusStr = "数据写入失败"
                    sself.progressView?.stopAnimating()
                    sself.progressView?.style = .error
                    break
                case .verify:
                    statusStr = "开始数据认证"
                    break
                case .verifyWait:
                    statusStr = "等待数据认证"
                    break
                case .verifySuccess:
                    statusStr = "数据认证成功"
                    break
                case .verifyFailed:
                    statusStr = "数据认证失败"
                    sself.progressView?.stopAnimating()
                    sself.progressView?.style = .error
                    break
                case .end:
                    statusStr = "升级成功"
                    sself.progressView?.style = .done
                    break
                default:
                    break
                }
                
                sself.progressView?.updateStatus(status: statusStr)
            }
        }
    }
    
    
    @IBAction func backGroundAction(_ sender: Any) {
        self.inputTextField.resignFirstResponder()
    }
    
    @objc func checkAction() {
        if (nil == self.checkView) {
            self.checkView = CH_UpdateCheckView.instanceFromNib()
        }
    
        self.view.addSubview(self.checkView!)
        self.checkView!.snp.makeConstraints { make in
            make.left.equalTo(0)
            make.right.equalTo(0)
            make.top.equalTo(0)
            make.bottom.equalTo(0)
        }
        
        let model = BLEOTAManager.shareInstance().getCurrentImageInfo()
        self.checkView!.updateCheckInfo(model:model!)
        
        self.checkView!.chipTypeSelect = { (index) in
            var type:ChipType?
            if (index == 0) {
                type = .CH573
            }else if (index == 1) {
                type = .CH583
            }else if (index == 2) {
                type = .CH579
            }
            BLEOTAManager.shareInstance().setChipType(type!)
        }
    }
}
