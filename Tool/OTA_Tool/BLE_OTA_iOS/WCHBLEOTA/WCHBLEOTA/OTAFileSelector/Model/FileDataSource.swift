//
//  FileDataSource.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import Foundation
import UIKit.UIImage

struct FileNodeRepresentation {
    var node: FileNode
    
    var level: Int
    var name: String
    var collapsed: Bool
    var size: Int
    var image: UIImage
    
    var modificationDate: Date?
    var sizeInfo: String
    var valid: Bool
    var isDirectory: Bool
}

struct FileDataSource {
    var items: [FileNodeRepresentation] = []
    var fileExtensionFilter: String?
    
    mutating func updateItems(_ dir: Directory) {
        items = items(dir, level: 0)
    }
    
    func items(_ dir: Directory, level: Int = 0) -> [FileNodeRepresentation] {
        dir.nodes.reduce([FileNodeRepresentation]()) { (result, node) in
            let valid: Bool
            if node is File, let ext = fileExtensionFilter, node.url.pathExtension != ext {
                valid = false
            } else {
                valid = true
            }
            
            var res = result
            let image = (node is Directory)
                ? ImageWrapper(icon: .folder, imageName: "folderEmpty").image
                : UIImage.icon(forFileURL: node.url, preferredSize: .smallest)
            
            let infoText: String
            if let dir = node as? Directory {
                infoText = "\(dir.nodes.count) items"
            } else {
                infoText = ByteCountFormatter().string(fromByteCount: Int64((node as! File).size))
            }
            
            res.append(FileNodeRepresentation(node: node, level: level, name: node.name, collapsed: false, size: 0, image: image!, modificationDate: node.resourceModificationDate, sizeInfo: infoText, valid: valid, isDirectory: (node is Directory)))
            if let dir = node as? Directory {
                res.append(contentsOf: self.items(dir, level: level + 1))
            }
            return res
        }
    }
}
