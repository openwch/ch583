//
//  DocumentFileManager.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import Foundation

class DocumentFileManager: FileManager {
    
    func buildDocumentDir() throws -> Directory {
        let docDir = try url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: false)
        return try buildDir(url: docDir)
    }
    
    func buildDir(url: URL) throws -> Directory {
        let nodes = try buildTree(url: url)
        return Directory(url: url, nodes: nodes)
    }
    
    func buildTree(url: URL) throws -> [FileNode] {
        do {
            return try contentsOfDirectory(atPath: url.path).compactMap { itemName -> FileNode? in
                let itemUrl = url.appendingPathComponent(itemName)
                
                var isDir: ObjCBool = false
                let fileExist = self.fileExists(atPath: itemUrl.path, isDirectory: &isDir)
                
                guard fileExist else { return nil }
                
                let attr = try attributesOfItem(atPath: itemUrl.path)
                let modificationDate = attr[.modificationDate] as? Date
                
                if isDir.boolValue {
                    let nestedNodes = try self.buildTree(url: itemUrl)
                    return Directory(url: itemUrl, nodes: nestedNodes, resourceModificationDate: modificationDate)
                } else {
                    let size = attr[.size] as? Int ?? 0
                    return File(url: itemUrl, size: size, resourceModificationDate: modificationDate)
                }
            }
        } catch {
            return []
        }
    }
    
    func deleteNode(_ node: FileNode) throws {
        try removeItem(at: node.url)
    }
}
