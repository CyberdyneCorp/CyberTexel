// An empty shell. XCTest needs a host application to inject its bundle into on
// iOS; all the work lives in ProbeTests, which reports back through the result
// bundle. The host does nothing so that nothing it does can fail the run.
//
// Two things about iOS 27 cost real time here and are worth stating plainly:
//
//  - `@main` on a UIApplicationDelegate did not register the delegate. A
//    fatalError as the first statement of didFinishLaunching never fired and
//    the app exited 0. Naming the principal class to UIApplicationMain works.
//  - iOS 27 TRAPS an app that does not adopt the scene lifecycle, inside
//    __UIApplicationEvaluateRuntimeIssueForNoSceneLifecycleAdoption. A scene
//    manifest whose UISceneConfigurations is an empty dictionary is not
//    adoption: it needs a real configuration naming a scene delegate class.
import UIKit

@objc(SceneDelegate)
final class SceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow?

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession,
               options: UIScene.ConnectionOptions) {
        guard let windowScene = scene as? UIWindowScene else { return }
        let window = UIWindow(windowScene: windowScene)
        window.rootViewController = UIViewController()
        window.makeKeyAndVisible()
        self.window = window
    }
}

@objc(AppDelegate)
final class AppDelegate: UIResponder, UIApplicationDelegate {
    func application(_ application: UIApplication,
                     didFinishLaunchingWithOptions options: [UIApplication.LaunchOptionsKey: Any]?)
        -> Bool
    { true }

    func application(_ application: UIApplication,
                     configurationForConnecting session: UISceneSession,
                     options: UIScene.ConnectionOptions) -> UISceneConfiguration {
        let configuration = UISceneConfiguration(name: "Default",
                                                 sessionRole: session.role)
        configuration.delegateClass = SceneDelegate.self
        return configuration
    }
}

UIApplicationMain(CommandLine.argc, CommandLine.unsafeArgv, nil,
                  NSStringFromClass(AppDelegate.self))
