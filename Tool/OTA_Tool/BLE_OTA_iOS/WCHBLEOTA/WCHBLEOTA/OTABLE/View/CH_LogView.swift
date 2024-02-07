//
//  LogView.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/24.
//

import UIKit

class CH_LogView: UIView {

    @IBOutlet weak var tableView: UITableView!

    private var logArray:NSArray?
    
    override func awakeFromNib() {
        self.tableView.delegate = self
        self.tableView.dataSource = self
        self.tableView.separatorStyle = .none
        self.tableView.estimatedRowHeight = 44.0
        self.tableView.rowHeight = UITableView.automaticDimension
        self.tableView.tableFooterView = UIView()
    }
    
    public func setLog(log:NSArray?) {
        if (log == nil) {
            return
        }
        self.logArray = log
        self.tableView.reloadData()
    }

}

extension CH_LogView:UITableViewDelegate, UITableViewDataSource {

    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return self.logArray?.count ?? 0
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        var cell = tableView.dequeueReusableCell(withIdentifier:"logCell")
        if (cell == nil) {
            cell = UITableViewCell.init(style: .default, reuseIdentifier: "logCell")
        }
        cell?.selectionStyle = .none
        if ((self.logArray?.count ?? 0) > indexPath.row) {
            let log = self.logArray?[indexPath.row] as? String ?? ""
            cell?.textLabel?.numberOfLines = 0
            cell?.textLabel?.font = UIFont.systemFont(ofSize: 14.0)
            cell?.textLabel?.text = log
        }
        return cell ?? UITableViewCell()
    }
}
