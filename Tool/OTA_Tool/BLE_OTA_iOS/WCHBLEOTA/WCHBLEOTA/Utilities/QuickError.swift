//
//  QuickError.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import Foundation

protocol ReadableError: Error {
    var title: String { get }
    var readableMessage: String { get }
}

extension ReadableError {
    var title: String {
        return "Error"
    }
}

struct QuickError: ReadableError {
    let readableMessage: String
    
    init(message: String) {
        readableMessage = message
    }
}
