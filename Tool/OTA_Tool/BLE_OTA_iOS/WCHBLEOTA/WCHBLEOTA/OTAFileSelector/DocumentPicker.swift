//
//  DocumentPicker.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import UIKit
import UniformTypeIdentifiers

class DocumentPicker<T>: NSObject, UIDocumentPickerDelegate {
    typealias Callback = (Result<T, Error>) -> ()
    private (set) var callback: Callback!
    let types: [String]
    
    init(documentTypes: [String]) {
        types = documentTypes
        super.init()
    }
    
    func openDocumentPicker(presentOn controller: UIViewController, callback: @escaping Callback) {
        let documentPickerVC: UIDocumentPickerViewController
        if #available(iOS 14.0, *) {
            //documentPickerVC = UIDocumentPickerViewController(forOpeningContentTypes: self.types.map { UTTypeReference(importedAs: $0) as UTType })
           
            var contentType:[UTType] = []
            
            for item in self.types {
                let type = UTType.init(item)
                if (type != nil) {
                    contentType.append(type!)
                }
            }
            documentPickerVC = UIDocumentPickerViewController(forOpeningContentTypes:contentType)
            
        } else {
            documentPickerVC = UIDocumentPickerViewController(documentTypes: types, in: .import)
        }
        documentPickerVC.delegate = self
        controller.present(documentPickerVC, animated: true)
        self.callback = callback
    }
    
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentAt url: URL) {


    }
}

class WCHDocumentPicker:DocumentPicker<File> {
    
    init() {
        super.init(documentTypes: ["public.data", "com.pkware.zip-archive"])
    }
    
    override func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentAt url: URL) {
        
        guard url.startAccessingSecurityScopedResource() else {
            // Handle the failure here.
            return
        }
        
        defer { url.stopAccessingSecurityScopedResource() }
        
        var error: NSError? = nil
        NSFileCoordinator().coordinate(readingItemAt: url, error: &error) { (url) in
            
            let ext = url.pathExtension
            
            if ext.caseInsensitiveCompare("bin") != .orderedSame &&
                ext.caseInsensitiveCompare("hex") != .orderedSame
            {
                callback(.failure(QuickError(message: "Invalid Firmware type please use hex and bin")))
            }else {
                
                let file = File.init(url: url, size: 0)
                callback(.success(file))
            }
        }
    }
}
