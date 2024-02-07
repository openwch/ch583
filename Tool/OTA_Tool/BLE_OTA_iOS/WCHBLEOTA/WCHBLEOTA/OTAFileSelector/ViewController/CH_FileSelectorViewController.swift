//
//  CH_FileSelectorViewController.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import UIKit
import XappKit
import BLEOTALibrary

class CH_FileSelectorViewController<T>: MH_VC_BaseViewController, AlertPresenter, UITableViewDataSource, UITableViewDelegate {

    
    private let documentPicker: DocumentPicker<T>
    
    private var documentFileManager = DocumentFileManager()
    private (set) var dataSource = FileDataSource()
    
    
    var filterExtension: String? = nil  {
        didSet {
            dataSource.fileExtensionFilter = filterExtension
        }
    }
    
    @IBOutlet private var emptyView: UIView!
    @IBOutlet private var tableView: UITableView!
    @IBOutlet private var selectButton: UIButton!
    @IBOutlet private var docImage: UIImageView!
    
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    init(documentPicker: DocumentPicker<T>) {
        self.documentPicker = documentPicker
        super.init(nibName: "CH_FileSelectorViewController", bundle: .main)
        navigationItem.title = "Select Package"
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        _=self.leftBarItem(with: UIImage.init(named: "btn_common_back"), selector: #selector(backAction));
        tableView.register(UINib(nibName: "CH_FileTableViewCell", bundle: .main), forCellReuseIdentifier: "CH_FileTableViewCell")
        tableView.register( CH_ActionTableViewCell.self, forCellReuseIdentifier: "CH_ActionTableViewCell")
        

        NotificationCenter.default.addObserver(self, selector: #selector(reloadData), name: UIApplication.willEnterForegroundNotification, object: nil)
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .refresh, target: self, action: #selector(reloadData))
        
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        reloadData()
    }
    
    private func reloadItemList() {
        do {
            let directory = try documentFileManager.buildDocumentDir()
            dataSource.updateItems(directory)
        } catch let error {
            displayErrorAlert(error: error)
            return
        }
    }
    
    @objc
    func reloadData() {
        reloadItemList()
        
        if !dataSource.items.isEmpty {
            view = tableView
            tableView.reloadData()
            tableView.backgroundColor = .white
        } else {
            view = emptyView
        }
    }
    
    func documentWasOpened(document: T) {
        let file:File? = document as? File
        self.OTAStartUpdate(file: file)
    }

    
    func fileWasSelected(file: File) {
        self.OTAStartUpdate(file: file)
    }
    
    func OTAStartUpdate(file:File?) {
        
        if (file == nil || file?.url == nil) {
            _=self.showFailedMessage("文件不存在");
        }
        
        let ext = file?.url.pathExtension
        let name = file?.url.lastPathComponent
        
        if ext?.caseInsensitiveCompare("bin") == .orderedSame {
            guard let binData = try? Data.init(contentsOf: file!.url) else {
                _=self.showFailedMessage("数据格式转换错误");
                return;
            }
            
            let updateVC = CH_UpdateViewController.init(nibName: "CH_UpdateViewController", bundle: .main)
            updateVC.fname = name
            updateVC.fileData = binData
            updateVC.ishex = false;
            self.navigationController?.pushViewController(updateVC, animated: true);
            
        }else if ext?.caseInsensitiveCompare("hex") == .orderedSame {
            guard let hexData = try? Data.init(contentsOf: file!.url) else {
                _=self.showFailedMessage("数据格式转换错误");
                return;
            }
     
            guard let binData = BLEOTAManager.shareInstance().hex2BinData(hexData) else {return};
            
            let updateVC = CH_UpdateViewController.init(nibName: "CH_UpdateViewController", bundle: .main)
            updateVC.fname = name
            updateVC.fileData = binData
            updateVC.ishex = true;
            self.navigationController?.pushViewController(updateVC, animated: true);
        }else {
            _ = self.showFailedMessage("请使用hex或bin格式的文件")
        }
        

        
    }
    
    
    
    @IBAction private func openDocumentPicker() {
        documentPicker.openDocumentPicker(presentOn: self) { [unowned self] (result) in
            switch result {
            case .success(let result):
                self.documentWasOpened(document: result)
            case .failure(let error):
                self.displayErrorAlert(error: error)
            }
        }
    }

    func numberOfSections(in tableView: UITableView) -> Int {
        return 2
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        section == 0 ? dataSource.items.count : 1
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard indexPath.section == 0 else {
            let cell = tableView.dequeueReusableCell(withIdentifier: "CH_ActionTableViewCell")
            cell?.textLabel?.text = "Import Another"
            return cell ?? UITableViewCell()
        }
        
        let cell = tableView.dequeueReusableCell(withIdentifier: "CH_FileTableViewCell") as? CH_FileTableViewCell
        let item = dataSource.items[indexPath.row]
        cell?.update(item)
        return cell ?? UITableViewCell()
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        indexPath.section == 0
            ? UIDevice.current.userInterfaceIdiom == .pad
                ? 80
                : 66
            : 44
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if indexPath.section == 1 {
            openDocumentPicker()
        } else if let file = dataSource.items[indexPath.row].node as? File, dataSource.items[indexPath.row].valid {
            fileWasSelected(file: file)
        }
        
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        section == 0 ? "Documents Directory" : ""
    }
    
    func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        indexPath.section == 0
    }
    
    func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        guard editingStyle == .delete else { return }
        let item = self.dataSource.items[indexPath.row]
        
        let deleteItem: (FileNode) -> (Error?) = { [weak self] node in
            guard let `self` = self else { return QuickError(message: "Unknown Error") }
            
            do {
                try self.documentFileManager.deleteNode(node)
            } catch let error {
                return error
            }
            return nil
        }
        
        if let directory = item.node as? Directory {
            let alert = UIAlertController(title: "Remove Directory", message: "Do you want to delete entire directory with all nested items?", preferredStyle: .alert)
            let delete = UIAlertAction(title: "Delete", style: .destructive, handler: { _ in
                self.removeDirectory(directory, rootItem: item, deleteAction: deleteItem)
            })
            let cancel = UIAlertAction(title: "Cancel", style: .cancel, handler: nil)
            
            alert.addAction(delete)
            alert.addAction(cancel)
            
            self.present(alert, animated: true, completion: nil)
        } else {
            deleteFile(item.node, deleteAction: deleteItem)
        }
        
    }
    
    private func removeDirectory(_ dir: Directory, rootItem: FileNodeRepresentation, deleteAction: (FileNode) -> (Error?)) {
        let indexPaths = ([rootItem] + dataSource.items(dir))
            .compactMap { item in
                self.dataSource.items.firstIndex { item.node.url == $0.node.url }
            }
            .map { IndexPath(row: $0, section: 0)}
        if let error = deleteAction(rootItem.node) {
            displayErrorAlert(error: error)
            return
        }
        self.reloadItemList()
        tableView.deleteRows(at: indexPaths, with: .automatic)
    }
    
    private func deleteFile(_ item: FileNode, deleteAction: (FileNode) -> (Error?)) {
        guard let ip = self.dataSource.items
            .firstIndex (where: { item.url == $0.node.url })
            .map ({ IndexPath(row: $0, section: 0) }) else {
            return
        }
        
        if let error = deleteAction(item) {
            displayErrorAlert(error: error)
            return
        }
        self.reloadItemList()
        tableView.deleteRows(at: [ip], with: .automatic)
    }
}
