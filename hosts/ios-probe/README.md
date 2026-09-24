# iOS device probe

A minimal signed iOS app that runs CyberTexel's mobile route on real hardware
and reports what it found. It exists because CI only cross-compiles and
link-checks for iOS: nothing proves the library initialises and runs on a
device.

It is not a numbered example and not a reference host. It gathers evidence.

```
xcodebuild -project CyberTexelDeviceProbe.xcodeproj -target CyberTexelDeviceProbe \
    -configuration Release -destination "id=<UDID>" -allowProvisioningUpdates build
xcrun devicectl device install app --device <UDID> build/Release-iphoneos/CyberTexelDeviceProbe.app
xcrun devicectl device process launch --device <UDID> com.cyberdyne.cybertexel.deviceprobe
```

## What works

Building, signing, installing, launching and **executing** on device, against
both an iPhone 16 and an iPad Air 13-inch (M3). Three things had to be fixed to
get there, each recorded because each cost time:

- **The signing team id is the certificate's OU, not the identifier in its CN.**
  `Apple Development: Name (N229YJ7FZ2)` with `OU=2C69VJZSNR` signs for team
  `2C69VJZSNR`. Using the CN value produces "No Account for Team".
- **iOS tears down an app that returns without a window.** The app needs a
  `UIApplicationSceneManifest` and a real `UIWindow`, or it takes SIGTRAP during
  launch.
- **`@main` on a `UIApplicationDelegate` did not register the delegate.** Proved
  by putting `fatalError` as the first statement of
  `didFinishLaunchingWithOptions`: it never fired and the app exited 0. Calling
  `UIApplicationMain` with the principal class named explicitly wires it up, and
  the same `fatalError` then fires.

## What does not work yet

Reading the report back. The app writes JSON into its Documents directory and
`devicectl device info files --domain-type appDataContainer --search "*"`
reports zero files, on both devices, with `UIFileSharingEnabled` set. The
delegate demonstrably runs, so the write or the container view is the problem,
not the code path.

`--domain-type systemCrashLogs` **is** readable, which is how the delegate was
proved to run, but a Swift `fatalError` message does not survive into the `.ips`
so it cannot carry a payload.

The next thing to try is an XCTest target run through
`xcodebuild test -destination`, which returns results to the host in an
`.xcresult` and needs no container access at all. That is the standard way to
run code on a device and get data back, and it sidesteps this entirely.

## What it cannot do

Satisfy a budget. `device-gate` decides only figures from a device named in
`benchmarks/device_gate.json`, and reports anything else as informational. The
iPad Air is now the named tablet; the iPhone is not and never will be.
