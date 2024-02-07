//
//  CH_Preferences.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/11.
//

import UIKit

struct CH_Preferences: Codable {
    var needFilter: Bool
    var filter: Int
    
    init() {
        self.needFilter = false
        filter = -90
    }
}



class CH_PreferencesStore: NSObject {
    static let shared = CH_PreferencesStore()
    private let preferencesKey = "preferencesKey"
    
    var preferences: CH_Preferences? {
        didSet {
            self.save()
        }
    }
    
    override init() {
        if let data = UserDefaults.standard.object(forKey: preferencesKey) as? Data, let preferences = try? JSONDecoder().decode(CH_Preferences.self, from: data) {
            self.preferences = preferences
        }
    }
    
    private func save() {
        guard let encodedData = try? JSONEncoder().encode(preferences) else {
            return
        }
        
        UserDefaults.standard.set(encodedData, forKey: preferencesKey)
        UserDefaults.standard.synchronize()
    }
}
