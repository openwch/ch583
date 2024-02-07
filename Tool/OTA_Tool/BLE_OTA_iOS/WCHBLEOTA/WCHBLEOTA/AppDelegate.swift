//
//  AppDelegate.swift
//  WCHBLEOTA
//
//  Created by moyun on 2022/4/6.
//

import UIKit
import XappKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?
    
    var homeVC: CH_HomeViewController?
    

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.
        self.window = UIWindow.init(frame: UIScreen.main.bounds)
        AppDelegate.configNavigationBarStyle()
        
        self.homeVC = CH_HomeViewController.init(nibName: "CH_HomeViewController", bundle: nil)
        let navHome = XAPP_BaseNavController.init(rootViewController: self.homeVC!)
        self.window?.rootViewController = navHome
        self.window?.makeKeyAndVisible()
        
        return true
    }
    
    func application(_ app: UIApplication, open inputURL: URL, options: [UIApplication.OpenURLOptionsKey : Any] = [:]) -> Bool {
        // Ensure the URL is a file URL
        guard inputURL.isFileURL else { return false }
        
        return true
    }
    
    
    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    }
    
    static func configNavigationBarStyle () {
        //导航栏配置
        UINavigationBar.appearance().barTintColor = UIColor.init(r: 42, g: 140, b: 205)
        UINavigationBar.appearance().tintColor = UIColor.white //UIColor.init(hexString: "f8652b")
//        UINavigationBar.appearance().barStyle = UIBarStyle.black; //
        UINavigationBar.appearance().isTranslucent = false
        let titleColor = UIColor.white //UIColor.init(hexString: "fb652b")
        UINavigationBar.appearance().titleTextAttributes = [NSAttributedString.Key.foregroundColor : titleColor, NSAttributedString.Key.font : UIFont.systemFont(ofSize: 18.0)]
        
        UINavigationBar.appearance().setBackgroundImage(UIImage.init(named: "navgation_top_bg"), for: .default)

    }

}

