// swift-tools-version: 5.9

import PackageDescription

// SwiftPM releases derive their package version from the Git tag. This value
// records the native ABI version the package manifest is compatible with.
let cyberTexelVersion = "0.1.0"

let package = Package(
    name: "CyberTexel",
    platforms: [.macOS(.v13), .iOS(.v16)],
    products: [
        .library(name: "CyberTexel", targets: ["CyberTexel"])
    ],
    targets: [
        .systemLibrary(name: "CyberTexelC", pkgConfig: "cybertexel"),
        .target(name: "CyberTexel", dependencies: ["CyberTexelC"]),
        .executableTarget(name: "CyberTexelLinkCheck", dependencies: ["CyberTexel"]),
        .testTarget(name: "CyberTexelTests", dependencies: ["CyberTexel"])
    ]
)
