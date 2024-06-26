//
//  AlertPresenter.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/8.
//

import UIKit

protocol AlertPresenter {
    func displayErrorAlert(error: Error)
}

extension AlertPresenter where Self:UIViewController {
    func displayErrorAlert(error: Error) {
        let msg = (error as? ReadableError)?.readableMessage ?? error.localizedDescription
        let alertController = UIAlertController(title: "Error", message: msg, preferredStyle: .alert)
        let cancelAction = UIAlertAction(title: "Cancel", style: .cancel, handler: nil)
        alertController.addAction(cancelAction)
        present(alertController, animated: true, completion: nil)
    }
    
    func displayOnError<T>(_ expression: @autoclosure () throws -> T) throws -> T {
        do {
            return try expression()
        } catch let error {
            displayErrorAlert(error: error)
            throw error
        }
    }
}
