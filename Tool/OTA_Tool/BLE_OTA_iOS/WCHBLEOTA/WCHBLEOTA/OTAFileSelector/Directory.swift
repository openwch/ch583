//
//  Directory.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import Foundation

protocol FileNode {
    var url: URL { get }
    var name: String { get }
    var resourceModificationDate: Date? { get }
}

extension FileNode {
    var name: String {
        return url.lastPathComponent
    }
}

struct File: FileNode {
    let url: URL
    let size: Int
    var resourceModificationDate: Date?
}

struct Directory: FileNode {
    let url: URL
    var nodes: [FileNode]
    var resourceModificationDate: Date?
}
